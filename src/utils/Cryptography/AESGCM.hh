#pragma once

#include <array>
#include <cstdint>
#include <vector>

class AESGCM {
public:
    static constexpr std::size_t KEY_SIZE = 32;   
    static constexpr std::size_t NONCE_SIZE = 12; 
    static constexpr std::size_t TAG_SIZE = 16;

    using Key = std::array<std::uint8_t, KEY_SIZE>;

    struct EncryptedData {
        std::array<std::uint8_t, NONCE_SIZE> nonce;
        std::vector<std::uint8_t> ciphertext;
        std::array<std::uint8_t, TAG_SIZE> tag;
    };

    static EncryptedData encrypt(
        const Key& key,
        const std::vector<std::uint8_t>& plaintext);

    static std::vector<std::uint8_t> decrypt(
        const Key& key,
        const EncryptedData& encrypted);
};