cmake_minimum_required(VERSION 2.8)
project(glrender)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(SOURCE_FILES main.cc amath.h checkerror.h initshader.cc mat.h vec.h misc.h)

include_directories("/usr/include/GL")

add_executable(myprog ${SOURCE_FILES})
target_link_libraries(myprog glut GL GLU GLEW m)

file(COPY fshader_passthrough.glsl vshader.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})