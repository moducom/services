cmake_minimum_required(VERSION 3.10)
project(services_tests)

set(CMAKE_CXX_STANDARD 17)

include_directories(../services)
add_subdirectory(../services build)

include(${CMAKE_SOURCE_DIR}/../../lib/conan/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
find_package(Threads REQUIRED)

add_executable(services_tests main.cpp misc.cpp managers.cpp)
target_link_libraries(services_tests CONAN_PKG::entt CONAN_PKG::catch2 Threads::Threads)