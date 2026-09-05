cmake_minimum_required(VERSION 3.16)

project(LeakyChat-Phase5
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(utils)
add_subdirectory(chat)
