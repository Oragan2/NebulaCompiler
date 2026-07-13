cmake_minimum_required(VERSION 3.20)

project(NebulaCompiler LANGUAGES CXX)

add_compile_options(-fsanitize=address -g)

add_link_options(-fsanitize=address)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)