#include "critical_checker.h"

#include <format>
#include <ranges>

namespace s21 {

bool CriticalChecker::Compare(double value, const std::string& op,
                              double threshold) {
  constexpr double eps = 1e-9;
  if (op == "==") return std::abs(value - threshold) < eps;
  if (op == "<") return value < threshold;
  if (op == "<=") return value <= threshold;
  if (op == ">=") return value >= threshold;
  if (op == ">") return value > threshold;
  return false;
}

std::vector<std::string> CriticalChecker::Check(
    std::span<const Metric> metrics,
    std::span<const CriticalThreshold> thresholds) {
  std::vector<std::string> alerts;

  for (const auto& threshold : thresholds) {
    auto it = std::ranges::find(metrics, threshold.metric_name, &Metric::name);
    if (it == metrics.end()) continue;

    double val = 0.0;
    bool numeric = false;
    std::visit(
        [&val, &numeric](auto&& arg) {
          using T = std::remove_cvref_t<decltype(arg)>;
          if constexpr (std::is_arithmetic_v<T>) {
            val = static_cast<double>(arg);
            numeric = true;
          }
        },
        it->value);

    if (!numeric) continue;

    if (Compare(val, threshold.op, threshold.value)) {
      alerts.push_back(std::format("CRITICAL: {} = {:.2f} (threshold {} {})",
                                   it->name, val, threshold.op,
                                   threshold.value));
    }
  }

  return alerts;
}

}  // namespace s21
