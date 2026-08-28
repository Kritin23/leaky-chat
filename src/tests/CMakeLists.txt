add_executable(system_test
    system_test.cc
)

target_include_directories(system_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../chat
)

target_link_libraries(system_test
    PRIVATE
    utils
    chat
)

add_test(
    NAME system_test
    COMMAND system_test
)