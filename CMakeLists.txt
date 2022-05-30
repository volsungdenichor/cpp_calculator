cmake_minimum_required(VERSION 3.5)
project(cpp_calculator)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_subdirectory(src)
add_subdirectory(tests)

