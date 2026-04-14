cmake_minimum_required(VERSION 3.22.1)
project(stream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -pedantic)

add_executable(stream main.cpp)
