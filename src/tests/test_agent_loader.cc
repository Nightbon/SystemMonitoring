#include <gtest/gtest.h>

#include "model/agent_loader.h"

class AgentLoaderTest : public ::testing::Test {
 protected:
  s21::AgentLoader loader;

  const std::filesystem::path valid_so = "./libvalid_mock_agent.so";
  const std::filesystem::path invalid_so = "./libinvalid_mock_agent.so";
  const std::filesystem::path null_so = "./libnull_mock_agent.so";
  const std::filesystem::path missing_so = "./does_not_exist.so";
  const std::filesystem::path fake_config = "config.conf";
};

// 1. Тест успешной загрузки корректной библиотеки
// TEST_F(AgentLoaderTest, SuccessLoad) {
//   s21::LoadedAgent loaded;
//   EXPECT_NO_THROW({ loaded = loader.Load(valid_so, fake_config); });

//   // Проверяем, что объект агента внутри shared_ptr действительно создался
//   ASSERT_NE(loaded.agent, nullptr);
//   EXPECT_EQ(loaded.so_path, valid_so);
// }

// 2. Тест ошибки dlopen, если файла библиотеки не существует
TEST_F(AgentLoaderTest, DlopenFailedMissingFile) {
  EXPECT_THROW(
      { [[maybe_unused]] auto result = loader.Load(missing_so, fake_config); },
      std::runtime_error);

  try {
    [[maybe_unused]] auto result = loader.Load(missing_so, fake_config);
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("dlopen failed"), std::string::npos);
  }
}

// 3. Тест ошибки dlsym, если в .so нет функции create_agent
TEST_F(AgentLoaderTest, DlsymFailedMissingFunction) {
  EXPECT_THROW(
      { [[maybe_unused]] auto result = loader.Load(invalid_so, fake_config); },
      std::runtime_error);

  try {
    [[maybe_unused]] auto result = loader.Load(invalid_so, fake_config);
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("dlsym failed"), std::string::npos);
  }
}

// 4. Тест ошибки, если create_agent возвращает nullptr
TEST_F(AgentLoaderTest, CreateAgentReturnedNull) {
  bool exception_thrown = false;
  try {
    [[maybe_unused]] auto result = loader.Load(null_so, fake_config);
  } catch (const std::runtime_error& e) {
    exception_thrown = true;
    EXPECT_NE(std::string(e.what()).find("create_agent returned null"),
              std::string::npos);
  }
  EXPECT_TRUE(exception_thrown)
      << "Исключение std::runtime_error не было выброшено";
}
