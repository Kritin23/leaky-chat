# fill in this file with the CMake commands to build the Cryptography module
# files in this directory are src/utils/Cryptography/*.cc and src/utils/Cryptography/*.hh
add_library(cryptography STATIC
    DHKeyExchange.cc
    KeyDerivation.cc
    AESGCM.cc
    CryptoSession.cc
)

target_include_directories(cryptography
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

find_package(OpenSSL REQUIRED)
target_link_libraries(cryptography
    PUBLIC
        OpenSSL::Crypto
)

target_compile_options(cryptography PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
