cmake_minimum_required(VERSION 3.10)
project(services)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_CURRENT_SOURCE_DIR}/../services.cmake)

add_library(${PROJECT_NAME}
        agents.hpp
        library.cpp library.h
        service.hpp services.h

        include/moducom/semver.h
        include/moducom/portable_endian.h

        include/moducom/services/description.h
        include/moducom/services/agent.h
        include/moducom/services/managers.hpp
        include/moducom/services/status.h

        include/moducom/stop_token.h
        )

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PROJECT_NAME} CONAN_PKG::entt Threads::Threads)
