cmake_minimum_required(VERSION 3.10)
project(service-libusb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET services)
    add_subdirectory(../services build)
endif()

include(../services.cmake)

add_library(${PROJECT_NAME}
        acm.cpp acm.h
        usb.cpp usb.h
        wrapper.h)

# DEBT: So damn sloppy
#target_include_directories(${PROJECT_NAME} PUBLIC {$CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${ROOT_DIR})

target_link_libraries(${PROJECT_NAME}
        CONAN_PKG::entt CONAN_PKG::libusb
        Threads::Threads
        services
        )
