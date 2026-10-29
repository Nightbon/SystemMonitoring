#include "../../model/base_agent.h"

// Функция присутствует, но намеренно возвращает nullptr
extern "C" s21::IAgent* create_agent(const char* config_path) {
  (void)config_path;  // Гасим варнинг о неиспользуемой переменной
  return nullptr;
}
