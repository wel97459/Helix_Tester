cmake_minimum_required(VERSION 3.10)

set(EXECUTABLE_OUTPUT_PATH ".")
set(CMAKE_CXX_FLAGS "-O3")
SET(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-O3")
SET(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE RelWithDebInfo)


project(test)

add_library(usb480 SHARED
   libusbd480.c
)


add_executable(${CMAKE_PROJECT_NAME}
   test.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} usb480 usb m)


add_executable(plasma
   plasma.c
)

target_link_libraries(plasma usb480 usb m)