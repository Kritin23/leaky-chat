function(add_chat_test test_name)
    add_executable(${test_name}
        ${test_name}.cc
    )

    target_link_libraries(${test_name}
        PRIVATE
        utils
        chat
    )

    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
    )
endfunction()

add_chat_test(system_test)
add_chat_test(multiple_clients_test)