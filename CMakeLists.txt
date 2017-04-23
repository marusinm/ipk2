cmake_minimum_required(VERSION 3.5)
project(traceroute)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES traceroute.cpp)
add_executable(traceroute ${SOURCE_FILES})
#add_executable(traceroute.test test.cpp)