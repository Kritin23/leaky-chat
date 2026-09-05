#include "KeyDerivation.hh"

#include <openssl/evp.h>

#include <stdexcept>

std::vector<std::uint8_t>
KeyDerivation::deriveKey(
    const std::vector<std::uint8_t>& sharedSecret)
{
    std::vector<std::uint8_t> key(32);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx)
        throw std::runtime_error("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(
            ctx,
            sharedSecret.data(),
            sharedSecret.size()) != 1) {

        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 failed");
    }

    unsigned int length = 0;

    if (EVP_DigestFinal_ex(
            ctx,
            key.data(),
            &length) != 1 ||
        length != key.size()) {

        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 failed");
    }

    EVP_MD_CTX_free(ctx);

    return key;
}