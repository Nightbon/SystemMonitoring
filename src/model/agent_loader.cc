#include "agent_loader.h"

#include <dlfcn.h>

#include <format>
#include <stdexcept>

namespace s21 {

LoadedAgent AgentLoader::Load(const Path& so_path, const Path& config_path,
                              std::stop_token stop_token) {
  if (stop_token.stop_requested()) {
    throw std::runtime_error("Load aborted: stop requested");
  }

  void* handle = dlopen(so_path.c_str(), RTLD_LAZY);
  if (!handle) {
    const char* raw_err = dlerror();
    std::string error_msg =
        raw_err ? raw_err : "Unknown error or file not found";
    throw std::runtime_error(std::format("dlopen failed: {}", error_msg));
  }

  auto handle_guard = std::unique_ptr<void, int (*)(void*)>(handle, dlclose);

  if (stop_token.stop_requested()) {
    throw std::runtime_error("Load aborted: stop requested before dlsym");
  }

  using CreateFunc = IAgent* (*)(const char*);
  auto create_agent =
      reinterpret_cast<CreateFunc>(dlsym(handle_guard.get(), "create_agent"));
  if (!create_agent) {
    const char* raw_err = dlerror();
    std::string error_msg =
        raw_err ? raw_err : "Symbol 'create_agent' not found";
    throw std::runtime_error(std::format("dlsym failed: {}", error_msg));
  }

  if (stop_token.stop_requested()) {
    throw std::runtime_error(
        "Load aborted: stop requested before creating agent");
  }

  IAgent* raw_agent = create_agent(config_path.c_str());
  if (!raw_agent) {
    const char* raw_err = dlerror();
    std::string error_msg = raw_err ? raw_err : "create_agent returned null";
    throw std::runtime_error(std::format("create_agent failed: {}", error_msg));
  }

  std::shared_ptr<IAgent> agent(raw_agent,
                                [h = handle_guard.release()](IAgent* ptr) {
                                  delete ptr;
                                  dlclose(h);
                                });

  if (stop_token.stop_requested()) {
    throw std::runtime_error(
        "Load aborted: stop requested after agent creation");
  }

  return {.agent = agent, .so_path = so_path};
}

}  // namespace s21
