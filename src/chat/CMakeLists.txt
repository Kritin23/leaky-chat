add_executable(server
    server.cpp 
    server_main.cpp
)

add_executable(client
    client.cpp
    client_main.cpp 
)

target_include_directories(server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server 
    PRIVATE 
        utils
)
target_link_libraries(client 
    PRIVATE 
        utils 
)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
