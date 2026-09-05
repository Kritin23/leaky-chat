#pragma once

#include <openssl/bn.h>

#include <cstdint>
#include <vector>

class DHKeyExchange {
  private:
    BIGNUM* mPrivateKey;
    BIGNUM* mPublicKey;

    static BIGNUM* modExp(const BIGNUM* base,
                          const BIGNUM* exponent,
                          const BIGNUM* modulus);

    static const BIGNUM* prime();
    static const BIGNUM* generator();

  public:
    DHKeyExchange();
    ~DHKeyExchange();
    DHKeyExchange(DHKeyExchange&& other) noexcept;
    DHKeyExchange& operator=(DHKeyExchange&& other) noexcept;

    std::vector<std::uint8_t> getPublicKey() const;

    std::vector<std::uint8_t> computeSharedSecret(
        const std::vector<std::uint8_t>& peerPublicKey) const;

    
};