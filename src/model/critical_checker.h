#pragma once
#include <span>
#include <string>
#include <vector>

#include "agent_config.h"
#include "metric.h"

namespace s21 {

class CriticalChecker {
 public:
  [[nodiscard]] static bool Compare(double value, const std::string& op,
                                    double threshold);

  [[nodiscard]] static std::vector<std::string> Check(
      std::span<const Metric> metrics,
      std::span<const CriticalThreshold> thresholds);
};

}  // namespace s21
