cmake_minimum_required(VERSION 3.1)
project(iscore_component_executor_automation LANGUAGES CXX)

iscore_common_setup()
set(DEPS
      iscore_lib_base
      iscore_lib_process
      iscore_lib_device
      iscore_plugin_engine
      iscore_plugin_curve
      iscore_plugin_deviceexplorer
      iscore_plugin_automation)


set(HDRS
"${CMAKE_CURRENT_SOURCE_DIR}/Executor/Automation/Component.hpp"
)
set(SRCS
"${CMAKE_CURRENT_SOURCE_DIR}/Executor/Automation/Component.cpp"
)

iscore_add_component(
    "${PROJECT_NAME}"
    "${SRCS}" "${HDRS}"
    "${DEPS}"
    1
    "e235fe6c-ba3f-4d18-9ab4-c517c6aa6997")
