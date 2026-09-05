#pragma once

#include "AESGCM.hh"
#include "DHKeyExchange.hh"

#include <cstdint>
#include <vector>
#include <string>

class CryptoSession {
  private:
    DHKeyExchange mDH;
    AESGCM::Key mKey{};
    bool mEstablished = false;

  public:
    std::vector<std::uint8_t> getPublicKey() const;

    void establish(
        const std::vector<std::uint8_t>& peerPublicKey);

    bool isEstablished() const;

    AESGCM::EncryptedData encrypt(
        const std::vector<std::uint8_t>& plaintext) const;

    std::vector<std::uint8_t> decrypt(
        const AESGCM::EncryptedData& encrypted) const;
};
