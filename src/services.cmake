set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# Passively specify where our modules live
list(APPEND CMAKE_MODULE_PATH ${ROOT_DIR}/lib/conan)

find_package(Threads REQUIRED)
find_package(libusb 1.0.23 REQUIRED)
find_package(EnTT REQUIRED)