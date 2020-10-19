cmake_minimum_required(VERSION 3.10)
project(services)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/../../lib/conan/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(services library.cpp library.h service.hpp semver.h)
target_link_libraries(services CONAN_PKG::entt)