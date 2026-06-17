cmake_minimum_required(VERSION 3.20)

project(NebulaCompiler LANGUAGES CXX)

add_compile_options(-g)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)