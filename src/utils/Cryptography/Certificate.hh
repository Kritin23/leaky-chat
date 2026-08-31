#pragma once

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <cstdint>
#include <string>
#include <vector>

class Certificate {
  private:
    X509* mCertificate = nullptr;

  public:
    Certificate(const std::string& path);
    ~Certificate();

    Certificate(const Certificate&) = delete;
    Certificate& operator=(const Certificate&) = delete;

    bool verify(const std::string& caPath,
                const std::string& expectedIdentity) const;

    EVP_PKEY* getPublicKey() const;

    static std::vector<std::uint8_t> sign(
        const std::string& privateKeyPath,
        const std::vector<std::uint8_t>& data);

    bool verifySignature(const std::vector<std::uint8_t>& data,
                         const std::vector<std::uint8_t>& signature) const;
};