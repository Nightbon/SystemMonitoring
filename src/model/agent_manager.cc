#include "agent_manager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <ranges>

namespace s21 {

namespace {

void EnsureAgentsDir(const Path& agents_dir) {
  if (fs::exists(agents_dir)) {
    if (fs::is_regular_file(agents_dir) || !fs::is_directory(agents_dir)) {
      fs::remove(agents_dir);
    }
  }
  fs::create_directories(agents_dir);
}

}  // namespace

AgentManager::AgentManager(Path agents_dir)
    : agents_dir_(std::move(agents_dir)), scan_interval_(ScanAgentsInterval) {
  EnsureAgentsDir(agents_dir_);
}

AgentManager::~AgentManager() { StopScanning(); }

void AgentManager::StartScanning(int interval_seconds) {
  scan_interval_ = interval_seconds;
  scan_thread_ = std::jthread(&AgentManager::ScanLoop, this);
}

void AgentManager::StopScanning() { scan_thread_.request_stop(); }

void AgentManager::ScanAndLoad(std::stop_token stop_token) {
  if (stop_token.stop_requested()) return;

  std::error_code ec;
  fs::directory_iterator dir_it(
      agents_dir_, fs::directory_options::skip_permission_denied, ec);
  if (ec) return;

  auto to_load = dir_it |
                 std::views::filter([stop_token](const fs::directory_entry&) {
                   return !stop_token.stop_requested();
                 }) |
                 std::views::filter([](const fs::directory_entry& entry) {
                   return entry.path().extension() == ".so";
                 }) |
                 std::views::filter([this](const fs::directory_entry& entry) {
                   return !IsAlreadyLoaded(entry.path());
                 }) |
                 std::views::transform([](const fs::directory_entry& entry) {
                   const auto& so_path = entry.path();
                   Path config_path = so_path;
                   config_path.replace_extension(".conf");
                   return std::make_pair(so_path, std::move(config_path));
                 });

  for (const auto& [so_path, config_path] : to_load) {
    if (stop_token.stop_requested()) [[unlikely]]
      break;

    try {
      auto loaded = loader_.Load(so_path, config_path, stop_token);
      std::unique_lock lock(mutex_);
      agents_.push_back(std::move(loaded));
    } catch (...) {
      // Игнорируем проблемные плагины
    }
  }
}

std::vector<std::shared_ptr<IAgent>> AgentManager::GetAgents() const {
  std::shared_lock lock(mutex_);
  auto view = agents_ | std::views::transform(
                            [](const LoadedAgent& la) { return la.agent; });
  return {view.begin(), view.end()};
}

void AgentManager::DisableAgent(const std::string& name) {
  std::unique_lock lock(mutex_);

  auto it = std::ranges::find_if(agents_, [&name](const LoadedAgent& loaded) {
    return loaded.agent && loaded.agent->GetName() == name;
  });

  if (it != agents_.end()) {
    it->agent->SetActive(!it->agent->IsActive());
  }
}

void AgentManager::ScanLoop(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    ScanAndLoad(stop_token);

    std::unique_lock<std::mutex> lock(cv_m_);
    cv_.wait_for(lock, stop_token, chr::seconds(scan_interval_),
                 [&stop_token] { return stop_token.stop_requested(); });
  }
}

bool AgentManager::IsAlreadyLoaded(const Path& path) const {
  std::shared_lock lock(mutex_);
  return std::ranges::find(agents_, path, &LoadedAgent::so_path) !=
         agents_.end();
}

}  // namespace s21
