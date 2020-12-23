cmake_minimum_required(VERSION 3.10)
project(service-libusb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(../services.cmake)

if(NOT TARGET services)
    add_subdirectory(../services build)
endif()

add_library(${PROJECT_NAME}
        acm.cpp acm.h
        include/moducom/libusb.h
        include/moducom/libusb/scoped.h
        include/moducom/libusb/service.h
        include/moducom/libusb/service.hpp
        include/moducom/libusb/wrapper.h
        usb.cpp
        )

add_library(Moducom::Service::LibUsb ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${ROOT_DIR}/src)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME}
        EnTT::EnTT libusb::libusb
        Threads::Threads
        Moducom::Services
        )
