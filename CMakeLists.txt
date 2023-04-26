cmake_minimum_required(VERSION 3.20)
project(reffub C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_COMPILER "g++-13")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++23 -fconcepts-diagnostics-depth=3")

include_directories(./include)
add_executable(reffub main.cpp)
