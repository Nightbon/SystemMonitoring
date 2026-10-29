#pragma once
#include <compare>
#include <string>
#include <variant>

namespace s21 {

using MetricValue = std::variant<int, double, std::string>;

struct Metric {
  std::string name;
  MetricValue value;

  auto operator<=>(const Metric&) const = default;
};

}  // namespace s21
