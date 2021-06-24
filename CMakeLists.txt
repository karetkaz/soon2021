cmake_minimum_required(VERSION 3.17)
project(Agent)

set(CMAKE_CXX_STANDARD 17)
# set the output directory for shared libraries to project dir
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)

if (WIN32)
    add_executable(Agent src/main.windows.cpp)
else()
    add_executable(Agent src/main.unix.cpp src/agent_config.cpp src/agent_mqtt.cpp)
    target_link_libraries(Agent dl)
endif()

add_library(tempAgent SHARED src/agent_temp.cpp)
target_include_directories(tempAgent PUBLIC .)

add_library(testAgent SHARED src/agent_test.cpp)
target_include_directories(tempAgent PUBLIC .)

add_library(mqttAgent SHARED src/agent_mqtt.cpp)
target_include_directories(mqttAgent PUBLIC .)
