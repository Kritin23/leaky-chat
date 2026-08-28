add_library(chat
    client.cpp
    server.cpp
)

target_include_directories(chat
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(chat
    PUBLIC
    utils
)