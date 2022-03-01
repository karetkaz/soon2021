cmake_minimum_required(VERSION 3.17)
project(Agent)

find_package(PythonLibs REQUIRED)

set(CMAKE_CXX_STANDARD 20)
link_directories(.)
include_directories(src/lib src/lib/mosquitto/include)
# set the output directory for shared libraries to project dir
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)

if (WIN32)
    add_executable(Agent src/main.windows.cpp src/core/agent_config.cpp)
else()
    add_executable(Agent src/main.cpp src/core/agent_config.cpp)
    target_link_libraries(Agent dl)
endif()

add_library(tempAgent SHARED src/agent/TempAgent.cpp)
target_include_directories(tempAgent PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(tempAgent ${PYTHON_LIBRARIES})

add_library(testAgent SHARED src/agent/TestAgent.cpp)
target_link_libraries(testAgent m pthread)

add_library(mqttAgent SHARED src/agent/MqttAgent.cpp)
target_link_libraries(mqttAgent mosquitto pthread)

add_library(execAgent SHARED src/agent/ExecAgent.cpp)
target_link_libraries(execAgent m pthread)
