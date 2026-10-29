#include <gtest/gtest.h>

#include "model/metric.h"

using namespace s21;

// ======================== MetricValue (variant) ========================

TEST(MetricValueTest, HoldsInt) {
  MetricValue v = 42;
  EXPECT_TRUE(std::holds_alternative<int>(v));
  EXPECT_EQ(std::get<int>(v), 42);
}

TEST(MetricValueTest, HoldsDouble) {
  MetricValue v = 3.14;
  EXPECT_TRUE(std::holds_alternative<double>(v));
  EXPECT_DOUBLE_EQ(std::get<double>(v), 3.14);
}

TEST(MetricValueTest, HoldsString) {
  MetricValue v = std::string("hello");
  EXPECT_TRUE(std::holds_alternative<std::string>(v));
  EXPECT_EQ(std::get<std::string>(v), "hello");
}

// ======================== Metric struct ========================

TEST(MetricTest, DefaultConstruction) {
  Metric m;
  EXPECT_TRUE(m.name.empty());
  EXPECT_TRUE(std::holds_alternative<int>(m.value));
  EXPECT_EQ(std::get<int>(m.value), 0);
}

TEST(MetricTest, IntMetric) {
  Metric m{"processes", 42};
  EXPECT_EQ(m.name, "processes");
  EXPECT_EQ(std::get<int>(m.value), 42);
}

TEST(MetricTest, DoubleMetric) {
  Metric m{"cpu", 87.5};
  EXPECT_EQ(m.name, "cpu");
  EXPECT_DOUBLE_EQ(std::get<double>(m.value), 87.5);
}

TEST(MetricTest, StringMetric) {
  Metric m{"status", std::string("OK")};
  EXPECT_EQ(m.name, "status");
  EXPECT_EQ(std::get<std::string>(m.value), "OK");
}

TEST(MetricTest, Equality) {
  Metric a{"cpu", 50.0};
  Metric b{"cpu", 50.0};
  Metric c{"cpu", 51.0};
  Metric d{"mem", 50.0};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_NE(a, d);
}

TEST(MetricTest, LessThanOrdering) {
  Metric a{"a_metric", 1};
  Metric b{"b_metric", 2};
  EXPECT_LT(a, b);

  Metric c{"cpu", 10.0};
  Metric d{"cpu", 20.0};
  EXPECT_LT(c, d);
}

TEST(MetricTest, SpaceshipThreeWayComparison) {
  Metric a{"x", 5};
  Metric b{"x", 5};
  Metric c{"x", 6};

  EXPECT_EQ(a <=> b, std::strong_ordering::equal);
  EXPECT_EQ(a <=> c, std::strong_ordering::less);
  EXPECT_EQ(c <=> a, std::strong_ordering::greater);
}

TEST(MetricTest, NegativeValues) {
  Metric m{"temp", -10.5};
  EXPECT_DOUBLE_EQ(std::get<double>(m.value), -10.5);
}

TEST(MetricTest, ZeroValues) {
  Metric m_int{"count", 0};
  Metric m_dbl{"ratio", 0.0};

  EXPECT_EQ(std::get<int>(m_int.value), 0);
  EXPECT_DOUBLE_EQ(std::get<double>(m_dbl.value), 0.0);
}

TEST(MetricTest, LargeValues) {
  Metric m{"big", 999999999};
  EXPECT_EQ(std::get<int>(m.value), 999999999);
}

TEST(MetricValueTest, VariantAssignment) {
  MetricValue v;
  v = 10;
  EXPECT_TRUE(std::holds_alternative<int>(v));
  v = 2.5;
  EXPECT_TRUE(std::holds_alternative<double>(v));
  v = std::string("text");
  EXPECT_TRUE(std::holds_alternative<std::string>(v));
}