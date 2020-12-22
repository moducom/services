cmake_minimum_required(VERSION 3.17)
project(terminal)

set(CMAKE_CXX_STANDARD 17)

set(ROOT_DIR ../..)
add_subdirectory(${ROOT_DIR}/src/service.libusb service-libusb)

# Passively specify where our modules live
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/conan)

find_package(Threads REQUIRED)
#find_package(libusb 1.0.23 REQUIRED)
find_package(EnTT REQUIRED)
#find_package(ncurses REQUIRED)

# Sloppy, but we're getting better
include_directories(${ROOT_DIR}/src)

add_executable(terminal main.cpp)

target_link_libraries(${PROJECT_NAME}
        ncurses
        EnTT::EnTT
        Moducom::Service::LibUsb)
