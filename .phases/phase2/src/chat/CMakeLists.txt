add_library(chat
    client.cpp
    server.cpp
    ui.cpp
)

target_include_directories(chat
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(chat
  PUBLIC
    utils
)

add_executable(client 
    client_main.cpp)

target_link_libraries(client 
  PUBLIC 
    utils 
    chat)

add_executable(server 
    server_main.cpp)
    
target_link_libraries(server 
  PUBLIC 
    utils 
    chat)

add_executable(mallory
    mallory/main.cpp
    mallory/MalloryProxy.cpp
)

target_include_directories(mallory PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(mallory
    PUBLIC
    utils
)