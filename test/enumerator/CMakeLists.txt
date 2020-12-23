cmake_minimum_required(VERSION 3.12)
project(enumerator)

set(CMAKE_CXX_STANDARD 17)

set(ROOT_DIR ../..)
add_subdirectory(${ROOT_DIR}/src/service.libusb service-libusb)

# Passively specify where our modules live
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/conan)

# Sloppy, but we're getting better
include_directories(${ROOT_DIR}/src)

add_executable(enumerator main.cpp)

target_link_libraries(${PROJECT_NAME}
        Moducom::Service::LibUsb)