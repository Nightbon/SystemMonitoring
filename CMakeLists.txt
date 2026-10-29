cmake_minimum_required(VERSION 3.16)
project(SystemMonitor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(CURL REQUIRED)

set(MODEL_SOURCES
    src/model/base_agent.cc
    src/model/agent_loader.cc
    src/model/agent_manager.cc
    src/model/config_parser.cc
    src/model/critical_checker.cc
    src/model/logger.cc
    src/model/monitor_core.cc
    src/model/alert_notifier.cc
    src/model/email_sender.cc
    src/model/telegram_bot.cc
)

set(MODEL_HEADERS
    src/model/base_agent.h
    src/model/agent_config.h
    src/model/agent_loader.h
    src/model/agent_manager.h
    src/model/config_parser.h
    src/model/critical_checker.h
    src/model/i_agent.h
    src/model/logger.h
    src/model/metric.h
    src/model/monitor_core.h
    src/model/alert_notifier.h
    src/model/email_sender.h
    src/model/telegram_bot.h
)

set(CONTROLLER_SOURCES
    src/controller/monitor_controller.cc
)

set(CONTROLLER_HEADERS
    src/controller/monitor_controller.h
)

set(VIEW_SOURCES
    src/view/agent_config_dialog.cc
    src/view/main_window.cc
    src/view/settings_dialog.cc
)

set(VIEW_HEADERS
    src/view/agent_config_dialog.h
    src/view/main_window.h
    src/view/settings_dialog.h
)

set(MAIN_SOURCE
    src/main.cc
)

set(ALL_SOURCES
    ${MODEL_SOURCES}
    ${CONTROLLER_SOURCES}
    ${VIEW_SOURCES}
    ${MAIN_SOURCE}
)

set(ALL_HEADERS
    ${MODEL_HEADERS}
    ${CONTROLLER_HEADERS}
    ${VIEW_HEADERS}
)

option(Release "Build release" OFF)
if(Release)
    add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

    set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS TRUE)

    target_include_directories(${PROJECT_NAME} PRIVATE src)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        dl
        pthread
        CURL::libcurl
    )
endif()

# ================ ТЕСТЫ ========================
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    # add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    # add_link_options(-fsanitize=address)

    find_package(GTest CONFIG REQUIRED COMPONENTS GTest GMock)
    find_package(Qt6 REQUIRED COMPONENTS Core)

    add_library(valid_mock_agent SHARED src/tests/fixtures/valid_mock_agent.cc)
    add_library(invalid_mock_agent SHARED src/tests/fixtures/invalid_mock_agent.cc)
    add_library(null_mock_agent SHARED src/tests/fixtures/null_mock_agent.cc)
    add_library(test_plugin_agent SHARED src/tests/fixtures/test_plugin_agent.cc)
    target_link_libraries(test_plugin_agent PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/model/config_parser.cc 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/model/base_agent.cc -std=c++20
    )

    set(TEST_SOURCES
    src/tests/test_agent_config.cc
    src/tests/test_agent_manager.cc
    src/tests/test_alert_notifier.cc
    src/tests/test_base_agent.cc
    src/tests/test_config_parser.cc
    src/tests/test_critical_checker.cc
    src/tests/test_email_sender.cc
    src/tests/test_logger.cc
    src/tests/test_main.cc
    src/tests/test_metric.cc
    src/tests/test_monitor_core.cc
    src/tests/test_telegram_bot.cc
    src/tests/test_agent_loader.cc
    )

    add_executable(run_tests
        ${TEST_SOURCES}
        ${MODEL_SOURCES}
        ${MODEL_HEADERS}
    )

    set_target_properties(run_tests PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )

    target_include_directories(run_tests PRIVATE src)

    target_link_libraries(run_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Qt6::Core
        dl
        pthread
        CURL::libcurl
    )

    add_dependencies(run_tests valid_mock_agent invalid_mock_agent null_mock_agent test_plugin_agent)

    enable_testing()
    add_test(NAME AllTests COMMAND run_tests)
    target_compile_definitions(run_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ================ Документация ========================
option(BUILD_DVI "Build docs" OFF)

if(BUILD_DVI)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_NAME "SystemMonitor")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_OUTPUT_LANGUAGE Russian)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_CALL_GRAPH YES)
        set(DOXYGEN_HAVE_DOT YES)
        set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
        set(DOXYGEN_SOURCE_BROWSER YES)
        set(DOXYGEN_QUIET YES)

        set(DOXYGEN_EXCLUDE_PATTERNS "*test*")

        doxygen_add_docs(doxygen
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ALL
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
        )
    endif()
endif()

# ================ АГЕНТЫ ========================
add_custom_target(agents
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR}/agents all
    COMMENT "Building agents"
)
