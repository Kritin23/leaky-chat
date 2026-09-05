#pragma once

#include <cstdint>
#include <vector>

class KeyDerivation {
public:
    static std::vector<std::uint8_t> deriveKey(
        const std::vector<std::uint8_t>& sharedSecret);
};