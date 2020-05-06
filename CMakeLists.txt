cmake_minimum_required(VERSION 3.16)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 14)

add_executable(ThreadPool main.cpp structure.h)
add_executable(test Test.cpp)