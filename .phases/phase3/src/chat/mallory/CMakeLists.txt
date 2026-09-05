cmake_minimum_required(VERSION 3.20)
project(phase3_mallory LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

add_executable(mallory
    main.cpp
    MalloryProxy.cpp
)

target_include_directories(mallory PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(mallory PRIVATE
    utils
    cryptography
    OpenSSL::Crypto
)
