cmake_minimum_required(VERSION 3.10)
project(services)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/../services.cmake)

add_library(${PROJECT_NAME}
        agents.hpp
        library.cpp library.h
        managers.hpp
        service.hpp services.h
        semver.h
        status.h
        stop_token.h)
target_link_libraries(${PROJECT_NAME} CONAN_PKG::entt Threads::Threads)
