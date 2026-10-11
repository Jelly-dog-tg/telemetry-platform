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