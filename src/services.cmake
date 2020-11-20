set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(${ROOT_DIR}/../lib/conan/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

find_package(Threads REQUIRED)
