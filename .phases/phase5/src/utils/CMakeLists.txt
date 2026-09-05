add_subdirectory(Cryptography)
add_library(utils STATIC
    MemBuffer.cpp
    NetworkHandler.cpp
    Packet.cpp
    SocketSet.cpp
    ConnectionSetter.cpp
    E2ESession.cc
)
target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../
)
target_link_libraries(utils
    PUBLIC
        cryptography
)
target_compile_options(utils PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)