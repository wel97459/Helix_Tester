cmake_minimum_required(VERSION 3.10)

# SET (CMAKE_C_COMPILER   "/usr/bin/clang")
# SET (CMAKE_CXX_COMPILER "/usr/bin/clang++")
#-fprofile-use=default.profdata -fprofile-generate
#set(MYAPP_PGO "-fprofile-generate") 
#set(MYAPP_PGO "-fprofile-use=../default.profdata") 

#set(MYAPP_PGO "-fprofile-generate=./pgo") 
#set(MYAPP_PGO "-fprofile-use=./pgo") 
if(NOT "${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_CXX_FLAGS "-DNDEBUG -O3 -march=native -flto ${MYAPP_PGO}")
set(CMAKE_EXE_LINKER_FLAGS "-Xlinker -Map=output.map ${MYAPP_PGO}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${MYAPP_PGO}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${MYAPP_PGO}")


SET(CMAKE_BUILD_TYPE Relese)


project(test)

add_library(usb480 SHARED
   libusbd480.c
)


add_executable(${CMAKE_PROJECT_NAME}
   test.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} usb480 usb m)
