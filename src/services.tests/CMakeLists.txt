cmake_minimum_required(VERSION 3.17)
project(services_tests)

set(CMAKE_CXX_STANDARD 14)

include_directories(../services)
add_subdirectory(../services build)

add_executable(services_tests main.cpp)