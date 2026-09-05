#include "Certificate.hh"

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

#include <cstdio>
#include <stdexcept>

Certificate::Certificate(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");

    if (!file)
        throw std::runtime_error("Failed to open certificate: " + path);

    mCertificate = PEM_read_X509(file, nullptr, nullptr, nullptr);

    fclose(file);

    if (!mCertificate)
        throw std::runtime_error("Failed to read certificate: " + path);
}

Certificate::Certificate(const std::vector<std::uint8_t>& data) {
    BIO* bio = BIO_new_mem_buf(data.data(), static_cast<int>(data.size()));

    if (!bio)
        throw std::runtime_error("BIO_new_mem_buf failed");

    mCertificate = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);

    BIO_free(bio);

    if (!mCertificate)
        throw std::runtime_error("Failed to parse certificate");
}

Certificate::~Certificate() {
    if (mCertificate)
        X509_free(mCertificate);
}

bool Certificate::verify(const std::string& caPath,
                         const std::string& expectedIdentity) const {
    X509_STORE* store = X509_STORE_new();

    if (!store)
        throw std::runtime_error("X509_STORE_new failed");

    if (X509_STORE_load_locations(store, caPath.c_str(), nullptr) != 1) {
        X509_STORE_free(store);

        throw std::runtime_error("Failed to load trusted CA certificate");
    }

    X509_STORE_CTX* ctx = X509_STORE_CTX_new();

    if (!ctx) {
        X509_STORE_free(store);

        throw std::runtime_error("X509_STORE_CTX_new failed");
    }

    if (X509_STORE_CTX_init(ctx, store, mCertificate, nullptr) != 1) {
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);

        throw std::runtime_error("X509_STORE_CTX_init failed");
    }

    if (X509_verify_cert(ctx) != 1) {
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        return false;
    }

    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);

    if (X509_check_ip_asc(mCertificate, expectedIdentity.c_str(), 0) != 1)
        return false;

    return true;
}

EVP_PKEY* Certificate::getPublicKey() const {
    if (!mCertificate)
        return nullptr;

    return X509_get_pubkey(mCertificate);
}

std::vector<std::uint8_t> Certificate::sign(
    const std::string& privateKeyPath,
    const std::vector<std::uint8_t>& data) {
    FILE* file = fopen(privateKeyPath.c_str(), "rb");

    if (!file) {
        throw std::runtime_error("Failed to open private key: " +
                                 privateKeyPath);
    }

    EVP_PKEY* privateKey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);

    fclose(file);

    if (!privateKey)
        throw std::runtime_error("Failed to read private key");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx) {
        EVP_PKEY_free(privateKey);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, privateKey) !=
        1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(privateKey);

        throw std::runtime_error("EVP_DigestSignInit failed");
    }

    if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(privateKey);

        throw std::runtime_error("EVP_DigestSignUpdate failed");
    }

    size_t signatureSize = 0;

    if (EVP_DigestSignFinal(ctx, nullptr, &signatureSize) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(privateKey);

        throw std::runtime_error("EVP_DigestSignFinal failed");
    }

    std::vector<std::uint8_t> signature(signatureSize);

    if (EVP_DigestSignFinal(ctx, signature.data(), &signatureSize) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(privateKey);

        throw std::runtime_error("EVP_DigestSignFinal failed");
    }

    signature.resize(signatureSize);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(privateKey);

    return signature;
}

bool Certificate::verifySignature(
    const std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& signature) const {
    EVP_PKEY* publicKey = getPublicKey();

    if (!publicKey)
        throw std::runtime_error("Failed to get certificate public key");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx) {
        EVP_PKEY_free(publicKey);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, publicKey) !=
        1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(publicKey);

        throw std::runtime_error("EVP_DigestVerifyInit failed");
    }

    if (EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(publicKey);

        throw std::runtime_error("EVP_DigestVerifyUpdate failed");
    }

    int result = EVP_DigestVerifyFinal(ctx, signature.data(), signature.size());

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(publicKey);

    return result == 1;
}