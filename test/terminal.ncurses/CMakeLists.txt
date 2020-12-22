cmake_minimum_required(VERSION 3.17)
project(terminal)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../../src/service.libusb service-libusb)

add_executable(terminal main.cpp)

target_link_libraries(${PROJECT_NAME} Moducom::Service::LibUsb)
