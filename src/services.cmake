set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH ${ROOT_DIR}/lib/conan)

#if(NOT DEFINED CONAN_LIB_DIRS)
#    include(${ROOT_DIR}/../lib/conan/conanbuildinfo.cmake)
#    conan_basic_setup(TARGETS KEEP_RPATHS)
#endif()

find_package(Threads REQUIRED)
find_package(libusb REQUIRED)
find_package(EnTT REQUIRED)