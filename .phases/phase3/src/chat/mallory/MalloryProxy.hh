#pragma once

#include "utils/NetworkHandler.hh"

#include <cstdint>
#include <string>
#include <vector>

class MalloryProxy {
  private:
    uint16_t mPort;
    std::string mServerHost;
    uint16_t mServerPort;

    std::string mFakeCertificatePath;
    std::string mLegitimateCertificatePath;
    std::string mMalloryPrivateKeyPath;

    int createListener() const;

    static bool sendAll(int socket, const void* data, size_t size);
    static bool receiveAll(int socket, void* data, size_t size);

    static bool sendVector(int socket,
                           const std::vector<std::uint8_t>& data);

    static bool receiveVector(int socket,
                              std::vector<std::uint8_t>& data);

    static std::vector<std::uint8_t> readFile(const std::string& path);

    static std::vector<std::uint8_t> sign(
        const std::string& privateKeyPath,
        const std::vector<std::uint8_t>& data);

  public:
    MalloryProxy(
        uint16_t port,
        std::string serverHost,
        uint16_t serverPort,
        std::string fakeCertificatePath,
        std::string legitimateCertificatePath,
        std::string malloryPrivateKeyPath)
        : mPort(port),
          mServerHost(std::move(serverHost)),
          mServerPort(serverPort),
          mFakeCertificatePath(std::move(fakeCertificatePath)),
          mLegitimateCertificatePath(std::move(legitimateCertificatePath)),
          mMalloryPrivateKeyPath(std::move(malloryPrivateKeyPath)) {}

    int runFakeCertificateAttack();
    int runStolenCertificateAttack();
};