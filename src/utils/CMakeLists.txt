add_library(utils STATIC
    MemBuffer.cpp
    Packet.cpp
    NetworkHandler.cpp
)

target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(utils PRIVATE -Wall -Wextra -Wpedantic)
