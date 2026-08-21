add_library(utils STATIC
    MemBuffer.cpp
)

target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(utils PRIVATE -Wall -Wextra -Wpedantic)
