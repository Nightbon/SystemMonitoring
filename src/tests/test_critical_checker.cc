#include <gtest/gtest.h>

#include "model/critical_checker.h"

using namespace s21;

// ======================== Compare ========================

class CompareTest : public ::testing::TestWithParam<
                        std::tuple<double, std::string, double, bool>> {};

TEST_P(CompareTest, VariousComparisons) {
  auto [value, op, threshold, expected] = GetParam();
  EXPECT_EQ(CriticalChecker::Compare(value, op, threshold), expected);
}

INSTANTIATE_TEST_SUITE_P(CompareOperations, CompareTest,
                         ::testing::Values(
                             // value, op, threshold, expected
                             std::make_tuple(5.0, "<", 10.0, true),
                             std::make_tuple(10.0, "<", 10.0, false),
                             std::make_tuple(15.0, "<", 10.0, false),

                             std::make_tuple(5.0, "<=", 10.0, true),
                             std::make_tuple(10.0, "<=", 10.0, true),
                             std::make_tuple(15.0, "<=", 10.0, false),

                             std::make_tuple(10.0, "==", 10.0, true),
                             std::make_tuple(9.0, "==", 10.0, false),
                             std::make_tuple(11.0, "==", 10.0, false),

                             std::make_tuple(15.0, ">=", 10.0, true),
                             std::make_tuple(10.0, ">=", 10.0, true),
                             std::make_tuple(5.0, ">=", 10.0, false),

                             std::make_tuple(15.0, ">", 10.0, true),
                             std::make_tuple(10.0, ">", 10.0, false),
                             std::make_tuple(5.0, ">", 10.0, false)));

TEST(CompareTest, UnknownOperatorReturnsFalse) {
  EXPECT_FALSE(CriticalChecker::Compare(100.0, "!=", 10.0));
  EXPECT_FALSE(CriticalChecker::Compare(100.0, "", 10.0));
  EXPECT_FALSE(CriticalChecker::Compare(100.0, "foo", 10.0));
}

TEST(CompareTest, NegativeValues) {
  EXPECT_TRUE(CriticalChecker::Compare(-5.0, "<", 0.0));
  EXPECT_TRUE(CriticalChecker::Compare(-10.0, ">", -20.0));
  EXPECT_TRUE(CriticalChecker::Compare(-5.0, "==", -5.0));
}

TEST(CompareTest, ZeroValues) {
  EXPECT_TRUE(CriticalChecker::Compare(0.0, "<=", 0.0));
  EXPECT_TRUE(CriticalChecker::Compare(0.0, ">=", 0.0));
  EXPECT_TRUE(CriticalChecker::Compare(0.0, "==", 0.0));
  EXPECT_FALSE(CriticalChecker::Compare(0.0, ">", 0.0));
  EXPECT_FALSE(CriticalChecker::Compare(0.0, "<", 0.0));
}

TEST(CompareTest, VerySmallValues) {
  EXPECT_TRUE(CriticalChecker::Compare(0.001, ">", 0.0));
  EXPECT_TRUE(CriticalChecker::Compare(0.0, "<", 0.001));
}

TEST(CompareTest, VeryLargeValues) {
  EXPECT_TRUE(CriticalChecker::Compare(1e15, ">", 1e10));
  EXPECT_FALSE(CriticalChecker::Compare(1e10, ">", 1e15));
}

// ======================== Check ========================

TEST(CheckTest, NoThresholdsNoAlerts) {
  std::vector<Metric> metrics = {{"cpu", 95.0}};
  std::vector<CriticalThreshold> thresholds;
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, NoMetricsNoAlerts) {
  std::vector<Metric> metrics;
  std::vector<CriticalThreshold> thresholds = {{"cpu", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, SingleThresholdTriggered) {
  std::vector<Metric> metrics = {{"cpu", 95.0}};
  std::vector<CriticalThreshold> thresholds = {{"cpu", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
  EXPECT_NE(alerts[0].find("CRITICAL"), std::string::npos);
  EXPECT_NE(alerts[0].find("cpu"), std::string::npos);
}

TEST(CheckTest, SingleThresholdNotTriggered) {
  std::vector<Metric> metrics = {{"cpu", 50.0}};
  std::vector<CriticalThreshold> thresholds = {{"cpu", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, MultipleMetricsOneThresholdTriggered) {
  std::vector<Metric> metrics = {{"cpu", 50.0}, {"ram", 95.0}};
  std::vector<CriticalThreshold> thresholds = {{"ram", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
  EXPECT_NE(alerts[0].find("ram"), std::string::npos);
}

TEST(CheckTest, MultipleThresholdsMultipleAlerts) {
  std::vector<Metric> metrics = {{"cpu", 95.0}, {"ram", 85.0}};
  std::vector<CriticalThreshold> thresholds = {
      {"cpu", ">=", 90.0},
      {"ram", ">=", 80.0},
  };
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 2u);
}

TEST(CheckTest, ThresholdMetricNotFoundSkipped) {
  std::vector<Metric> metrics = {{"cpu", 95.0}};
  std::vector<CriticalThreshold> thresholds = {{"nonexistent", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, StringMetricValueIgnored) {
  std::vector<Metric> metrics = {{"status", std::string("OK")}};
  std::vector<CriticalThreshold> thresholds = {{"status", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, IntMetricValueChecked) {
  std::vector<Metric> metrics = {{"processes", 600}};
  std::vector<CriticalThreshold> thresholds = {{"processes", ">", 500.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
}

TEST(CheckTest, IntMetricValueNotTriggered) {
  std::vector<Metric> metrics = {{"processes", 200}};
  std::vector<CriticalThreshold> thresholds = {{"processes", ">", 500.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, LessThanTriggered) {
  std::vector<Metric> metrics = {{"latency", 0.5}};
  std::vector<CriticalThreshold> thresholds = {{"latency", "<", 1.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
}

TEST(CheckTest, LessThanNotTriggered) {
  std::vector<Metric> metrics = {{"latency", 5.0}};
  std::vector<CriticalThreshold> thresholds = {{"latency", "<", 1.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, EqualTriggered) {
  std::vector<Metric> metrics = {{"val", 42.0}};
  std::vector<CriticalThreshold> thresholds = {{"val", "==", 42.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
}

TEST(CheckTest, AlertFormatContainsValues) {
  std::vector<Metric> metrics = {{"cpu", 95.5}};
  std::vector<CriticalThreshold> thresholds = {{"cpu", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
  EXPECT_NE(alerts[0].find("95.50"), std::string::npos);
  EXPECT_NE(alerts[0].find(">="), std::string::npos);
  // threshold.value форматируется без {:.2f} → "90" (не "90.00")
  EXPECT_NE(alerts[0].find("90"), std::string::npos);
}

TEST(CheckTest, MetricNameWithDot) {
  std::vector<Metric> metrics = {{"google.com", 0.0}};
  std::vector<CriticalThreshold> thresholds = {{"google.com", "<", 1.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  ASSERT_EQ(alerts.size(), 1u);
}

TEST(CheckTest, EmptyMetricsAndThresholds) {
  std::vector<Metric> metrics;
  std::vector<CriticalThreshold> thresholds;
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  EXPECT_TRUE(alerts.empty());
}

TEST(CheckTest, DuplicateMetricNamesFirstWins) {
  // Если в metrics есть дубликаты, ranges::find найдёт первый
  std::vector<Metric> metrics = {{"cpu", 50.0}, {"cpu", 95.0}};
  std::vector<CriticalThreshold> thresholds = {{"cpu", ">=", 90.0}};
  auto alerts = CriticalChecker::Check(metrics, thresholds);
  // Первый cpu=50 не превышает 90
  EXPECT_TRUE(alerts.empty());
}