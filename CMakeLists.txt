cmake_minimum_required(VERSION 3.16)

project(telemetry_platform LANGUAGES CXX)

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
