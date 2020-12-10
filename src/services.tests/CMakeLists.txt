cmake_minimum_required(VERSION 3.10)
project(services_tests)

set(CMAKE_CXX_STANDARD 17)

include_directories(..)
add_subdirectory(../services build)
add_subdirectory(../service.libusb libusb)

include(${CMAKE_SOURCE_DIR}/../services.cmake)

add_executable(services_tests
        agents.cpp
        depend.cpp
        main.cpp misc.cpp managers.cpp
        services.cpp services.h
        usb.cpp
        )

find_package(catch2 REQUIRED)

target_link_libraries(services_tests
        EnTT catch2::catch2
        Threads::Threads
        services
        service-libusb)