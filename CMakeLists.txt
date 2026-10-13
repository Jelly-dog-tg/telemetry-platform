cmake_minimum_required(VERSION 3.16)

project(telemetry_platform LANGUAGES CXX)
find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    telemetry
    main.cpp
    CsvStorage.cpp
    DeviceManager.cpp
)

enable_testing()

add_executable(
    telemetry_tests
    tests/DeviceManagerTests.cpp
    DeviceManager.cpp
)

target_include_directories(
    telemetry_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME DeviceManagerTests
    COMMAND telemetry_tests
)
add_executable(
    telemetry_update_tests
    tests/TelemetryUpdateTests.cpp
    DeviceManager.cpp
)

target_include_directories(
    telemetry_update_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME TelemetryUpdateTests
    COMMAND telemetry_update_tests
)
add_executable(
    tcp_server
    experiments/tcp_server.cpp
    CsvStorage.cpp
    DeviceManager.cpp
    ProtocolMessage.cpp
)

target_include_directories(
    tcp_server
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    tcp_server
    PRIVATE
    Threads::Threads
)
add_executable(
    protocol_message_tests
    tests/ProtocolMessageTests.cpp
    ProtocolMessage.cpp
)

target_include_directories(
    protocol_message_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME ProtocolMessageTests
    COMMAND protocol_message_tests
)
add_test(
    NAME TcpServerIntegration
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TcpServerIntegration.py
        $<TARGET_FILE:tcp_server>
)
set_tests_properties(
    TcpServerIntegration
    PROPERTIES TIMEOUT 20
)
