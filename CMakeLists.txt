cmake_minimum_required(VERSION 2.8)
project(glrender)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall --std=c++11")
set(SOURCE_FILES main.cc amath.h checkerror.h initshader.cc mat.h vec.h misc.h beziersurface.cc)

include_directories("/usr/include/GL")

add_executable(myprog ${SOURCE_FILES})
target_link_libraries(myprog glut GL GLU GLEW m)

file(COPY fshader.glsl vshader.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})