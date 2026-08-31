#include "AESGCM.hh"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <stdexcept>

AESGCM::EncryptedData AESGCM::encrypt(
    const Key& key,
    const std::vector<std::uint8_t>& plaintext)
{
    EncryptedData result{};

    // Fresh nonce for every message.
    if (RAND_bytes(
            result.nonce.data(),
            result.nonce.size()) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (EVP_EncryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            key.data(),
            result.nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES-GCM initialisation failed");
    }

    result.ciphertext.resize(plaintext.size());

    int written = 0;

    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(
                ctx,
                result.ciphertext.data(),
                &written,
                plaintext.data(),
                static_cast<int>(plaintext.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("AES-GCM encryption failed");
        }
    }

    int finalWritten = 0;

    if (EVP_EncryptFinal_ex(
            ctx,
            result.ciphertext.empty()
                ? nullptr
                : result.ciphertext.data() + written,
            &finalWritten) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES-GCM finalisation failed");
    }

    result.ciphertext.resize(written + finalWritten);

    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_GET_TAG,
            TAG_SIZE,
            result.tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get GCM tag");
    }

    EVP_CIPHER_CTX_free(ctx);

    return result;
}

std::vector<std::uint8_t> AESGCM::decrypt(
    const Key& key,
    const EncryptedData& encrypted)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (EVP_DecryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            key.data(),
            encrypted.nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES-GCM initialisation failed");
    }

    std::vector<std::uint8_t> plaintext(
        encrypted.ciphertext.size());

    int written = 0;

    if (!encrypted.ciphertext.empty()) {
        if (EVP_DecryptUpdate(
                ctx,
                plaintext.data(),
                &written,
                encrypted.ciphertext.data(),
                static_cast<int>(encrypted.ciphertext.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("AES-GCM decryption failed");
        }
    }

    if (EVP_CIPHER_CTX_ctrl(
        ctx,
        EVP_CTRL_GCM_SET_TAG,
        static_cast<int>(TAG_SIZE),
        const_cast<std::uint8_t*>(encrypted.tag.data())) != 1){
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM tag");
    }

    int finalWritten = 0;

    if (EVP_DecryptFinal_ex(
            ctx,
            plaintext.empty()
                ? nullptr
                : plaintext.data() + written,
            &finalWritten) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(
            "AES-GCM authentication failed");
    }

    plaintext.resize(written + finalWritten);

    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}
