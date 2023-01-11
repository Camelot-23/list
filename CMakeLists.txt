cmake_minimum_required(VERSION 3.12)

project(main LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# -Wall 打印警告信息
# -w  关闭警告信息
# -D  定义宏
# -O  基本优化
# -O0 不做优化
# -O3 最多优化
# -fPIC 编译动态库时，生成与位置无关的代码
# -fPIe 生成与位置无关的可执行代码
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -Wno-unused-result -g -DDebug")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wno-unused-result -g -DNoDebug")

if(CMAKe_BUILD_TYPE)
  message("Build type " ${CMAKe_BUILD_TYPE})
endif()

# The third party path
set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

# include-path
set(BASIC_INCLDUE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/Basic)
set(FUNCTIONAL_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/Functional)
set(MANAGEMENT_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/Management)

# src-path
set(BASIC_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Basic)
set(FUNCTIONAL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Functional)
set(MANAGEMENT_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/Management)

# paho-mqtt_c
set(MQTT_C_PATH ${THIRD_PARTY_PATH}/paho.mqtt.c)
include_directories(${MQTT_C_PATH}/build/_install/include)
find_library(PahoMqtt_A  NAMES paho-mqtt3a  HINTS ${MQTT_C_PATH}/build/_install/lib)
find_library(PahoMqtt_C  NAMES paho-mqtt3c  HINTS ${MQTT_C_PATH}/build/_install/lib)
find_library(PahoMqtt_AS NAMES paho-mqtt3as HINTS ${MQTT_C_PATH}/build/_install/lib)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  message("PahoMqtt_A : " ${PahoMqtt_A})
  message("PahoMqtt_C : " ${PahoMqtt_C})
  message("PahoMqtt_AS : " ${PahoMqtt_AS})
endif()

# paho-mqtt_cpp
set(MQTT_CPP_PATH ${THIRD_PARTY_PATH}/paho.mqtt.cpp)
include_directories(${MQTT_CPP_PATH}/build/_install/include)
find_library(PahoMqttCpp  NAMES paho-mqttpp3  HINTS ${MQTT_CPP_PATH}/build/_install/lib)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  message("PahoMqttCpp : " ${PahoMqttCpp})
endif()

include_directories(${BASIC_INCLDUE_PATH}/utils)


add_subdirectory(3rdparty/qn_audio)

find_library(QN_RTC NAMES qn_rtc HINTS ${THIRD_PARTY_PATH}/qn_audio/lib/x86_64)

# main
add_executable(
  ${PROJECT_NAME}
  src/main.cc
  ${BASIC_SRC_PATH}/utils/timer.cpp
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  ${PahoMqtt_A}
  # ${PahoMqtt_C}
  ${PahoMqtt_AS}
  ${PahoMqttCpp}

  ${QN_RTC}
  qn_audio
)

# target_compile_definitions(main PUBLIC Debug)