set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(CONAN_DISABLE_CHECK_COMPILER 1)

if(NOT DEFINED CONAN_LIB_DIRS)
    include(${ROOT_DIR}/../lib/conan/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS KEEP_RPATHS)
endif()

find_package(Threads REQUIRED)
