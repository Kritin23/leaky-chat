#include "MalloryProxy.hh"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils/MemBuffer.h"
#include "utils/Cryptography/Certificate.hh"

int MalloryProxy::createListener() const {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFd < 0) {
        perror("socket");
        return -1;
    }

    int enable = 1;

    setsockopt(
        socketFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &enable,
        sizeof(enable));

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(mPort);

    if (bind(
            socketFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) < 0) {
        perror("bind");
        close(socketFd);
        return -1;
    }

    if (listen(socketFd, SOMAXCONN) < 0) {
        perror("listen");
        close(socketFd);
        return -1;
    }

    return socketFd;
}

bool MalloryProxy::sendAll(int socket, const void* data, size_t size) {
    size_t sent = 0;

    const auto* bytes =
        static_cast<const std::uint8_t*>(data);

    while (sent < size) {
        ssize_t result =
            send(socket, bytes + sent, size - sent, 0);

        if (result > 0) {
            sent += static_cast<size_t>(result);
        } else if (result < 0 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }

    return true;
}

bool MalloryProxy::receiveAll(int socket, void* data, size_t size) {
    size_t received = 0;

    auto* bytes =
        static_cast<std::uint8_t*>(data);

    while (received < size) {
        ssize_t result =
            recv(socket, bytes + received, size - received, 0);

        if (result > 0) {
            received += static_cast<size_t>(result);
        } else if (result < 0 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }

    return true;
}

bool MalloryProxy::sendVector(
    int socket,
    const std::vector<std::uint8_t>& data) {

    MemBuffer buffer(1024);
    buffer << data;

    return sendAll(
        socket,
        buffer.data(),
        buffer.size());
}

bool MalloryProxy::receiveVector(
    int socket,
    std::vector<std::uint8_t>& data) {

    size_t size = 0;

    if (!receiveAll(socket, &size, sizeof(size))) {
        return false;
    }

    if (size > 64 * 1024) {
        return false;
    }

    data.resize(size);

    if (size == 0) {
        return true;
    }

    return receiveAll(
        socket,
        data.data(),
        data.size());
}

std::vector<std::uint8_t> MalloryProxy::readFile(
    const std::string& path) {

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        return {};
    }

    file.seekg(0, std::ios::end);

    std::streamsize size = file.tellg();

    file.seekg(0, std::ios::beg);

    if (size < 0) {
        return {};
    }

    std::vector<std::uint8_t> data(
        static_cast<size_t>(size));

    if (!data.empty()) {
        file.read(
            reinterpret_cast<char*>(data.data()),
            size);
    }

    if (!file) {
        return {};
    }

    return data;
}

std::vector<std::uint8_t> MalloryProxy::sign(
    const std::string& privateKeyPath,
    const std::vector<std::uint8_t>& data) {

    FILE* file =
        fopen(privateKeyPath.c_str(), "r");

    if (!file) {
        return {};
    }

    EVP_PKEY* privateKey =
        PEM_read_PrivateKey(
            file,
            nullptr,
            nullptr,
            nullptr);

    fclose(file);

    if (!privateKey) {
        return {};
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx) {
        EVP_PKEY_free(privateKey);
        return {};
    }

    size_t signatureSize = 0;

    if (EVP_DigestSignInit(
            ctx,
            nullptr,
            EVP_sha256(),
            nullptr,
            privateKey) != 1 ||
        EVP_DigestSignUpdate(
            ctx,
            data.data(),
            data.size()) != 1 ||
        EVP_DigestSignFinal(
            ctx,
            nullptr,
            &signatureSize) != 1) {

        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(privateKey);

        return {};
    }

    std::vector<std::uint8_t> signature(
        signatureSize);

    if (EVP_DigestSignFinal(
            ctx,
            signature.data(),
            &signatureSize) != 1) {

        signature.clear();
    } else {
        signature.resize(signatureSize);
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(privateKey);

    return signature;
}

int MalloryProxy::runFakeCertificateAttack() {
    auto certificate =
        readFile(mFakeCertificatePath);

    if (certificate.empty()) {
        std::cerr
            << "Failed to read fake certificate: "
            << mFakeCertificatePath
            << '\n';

        return 1;
    }

    int listener = createListener();

    if (listener < 0) {
        return 1;
    }

    std::cout
        << "Mallory listening on 0.0.0.0:"
        << mPort
        << " (fake certificate)\n";

    int clientSocket =
        accept(listener, nullptr, nullptr);

    if (clientSocket < 0) {
        perror("accept");
        close(listener);
        return 1;
    }

    std::cout
        << "Client connected. Sending fake certificate.\n";

    if (!sendVector(clientSocket, certificate)) {
        std::cerr << "Failed to send fake certificate.\n";
        close(clientSocket);
        close(listener);
        return 1;
    }

    std::cout
        << "Fake certificate sent.\n";

    close(clientSocket);
    close(listener);

    return 0;
}

int MalloryProxy::runStolenCertificateAttack() {
    auto certificate =
        readFile(mLegitimateCertificatePath);

    if (certificate.empty()) {
        std::cerr
            << "Failed to read legitimate server certificate: "
            << mLegitimateCertificatePath
            << '\n';

        return 1;
    }

    int listener = createListener();

    if (listener < 0) {
        return 1;
    }

    std::cout
        << "Mallory listening on 0.0.0.0:"
        << mPort
        << " (stolen certificate)\n";

    int clientSocket =
        accept(listener, nullptr, nullptr);

    if (clientSocket < 0) {
        perror("accept");
        close(listener);
        return 1;
    }

    std::cout
        << "Client connected. Sending legitimate server certificate.\n";

    if (!sendVector(clientSocket, certificate)) {
        std::cerr
            << "Failed to send legitimate certificate.\n";

        close(clientSocket);
        close(listener);

        return 1;
    }

    std::cout
        << "Waiting for client's challenge...\n";

    std::vector<std::uint8_t> challenge;

    if (!receiveVector(clientSocket, challenge)) {
        std::cerr
            << "Failed to receive challenge.\n";

        close(clientSocket);
        close(listener);

        return 1;
    }

    std::cout
        << "Challenge received: "
        << challenge.size()
        << " bytes\n";

    std::cout
        << "Signing challenge using Mallory's private key.\n";

    auto signature =
        sign(mMalloryPrivateKeyPath, challenge);

    if (signature.empty()) {
        std::cerr
            << "Failed to generate Mallory signature.\n";

        close(clientSocket);
        close(listener);

        return 1;
    }

    if (!sendVector(clientSocket, signature)) {
        std::cerr
            << "Failed to send Mallory signature.\n";

        close(clientSocket);
        close(listener);

        return 1;
    }

    std::cout
        << "Wrong signature sent.\n"
        << "Client should reject proof-of-possession.\n";

    close(clientSocket);
    close(listener);

    return 0;
}