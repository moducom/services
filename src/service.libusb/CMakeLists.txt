cmake_minimum_required(VERSION 3.10)
project(service-libusb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET services)
    add_subdirectory(../services build)
endif()

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/..)

include(${CMAKE_SOURCE_DIR}/../../lib/conan/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)
find_package(Threads REQUIRED)

add_library(${PROJECT_NAME}
        acm.cpp acm.h
        usb.cpp usb.h
        wrapper.h)

# DEBT: So damn sloppy
#target_include_directories(${PROJECT_NAME} PUBLIC {$CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${ROOT_DIR})

target_link_libraries(${PROJECT_NAME} CONAN_PKG::entt Threads::Threads CONAN_PKG::libusb)
