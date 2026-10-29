#include <gtest/gtest.h>

// Точка входа для всех GTest тестов
// gtest_main предоставляется GTest библиотекой

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}