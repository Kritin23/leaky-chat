#include "Certificate.hh"

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

    bool valid = X509_verify_cert(ctx) == 1;

    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);

    if (!valid)
        return false;

    if (X509_check_host(mCertificate,
                        expectedIdentity.c_str(),
                        expectedIdentity.size(),
                        0,
                        nullptr) != 1) {
        return false;
    }

    return true;
}

EVP_PKEY* Certificate::getPublicKey() const {
    if (!mCertificate)
        return nullptr;

    return X509_get_pubkey(mCertificate);
}