cmake_minimum_required(VERSION 3.10)
project(service-libusb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(../services.cmake)

if(NOT TARGET services)
    add_subdirectory(../services build)
endif()

add_library(${PROJECT_NAME}
        acm.cpp acm.h
        include/moducom/libusb/service.h
        include/moducom/libusb/service.hpp
        usb.cpp usb.h
        wrapper.h)

# DEBT: So damn sloppy
#target_include_directories(${PROJECT_NAME} PUBLIC {$CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${ROOT_DIR}/src)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME}
        EnTT::EnTT libusb::libusb
        Threads::Threads
        Moducom::Services
        )
