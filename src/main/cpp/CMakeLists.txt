cmake_minimum_required(VERSION 3.6)
project(unrar)


add_subdirectory(libunrar)
#add_executable(unrar main.cpp)
#target_link_libraries(unrar static_unrar)
include_directories(libunrar)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLITTLE_ENDIAN")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden")

add_library(unrar-jni SHARED rar_file.cpp jni_init.c)
target_link_libraries(unrar-jni static_unrar log)
