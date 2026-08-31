#pragma once

#include <string>

#include <openssl/x509.h>

class Certificate {
  private:
    X509* mCertificate = nullptr;

  public:
    Certificate(const std::string& path);
    ~Certificate();

    Certificate(const Certificate&) = delete;
    Certificate& operator=(const Certificate&) = delete;

    bool verify(
        const std::string& caPath,
        const std::string& expectedIdentity) const;

    EVP_PKEY* getPublicKey() const;
};