#include "NetworkHandler.hh"

#include <arpa/inet.h>
#include <openssl/rand.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "Cryptography/Certificate.hh"
#include "MemBuffer.h"

using namespace std::chrono_literals;

NetworkHandler::~NetworkHandler() {
    close();
}

ssize_t NetworkHandler::receiveBytes(void* buffer, size_t size) {
    ssize_t received = 0;
    while (received < static_cast<ssize_t>(size)) {
        ssize_t bytes = recv(
            mSocket, static_cast<char*>(buffer) + received, size - received, 0);
        if (bytes > 0) {
            received += bytes;
        } else if (bytes == 0) {
            return -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            return -1;
        } else {
            continue;
        }
    }

    return received;
}

int NetworkHandler::performClientCertificateAuthentication() {
    char buffer[64 * 1024];

    ssize_t bytes_received = recv(mSocket, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0) {
        std::cerr << "Failed to receive server certificate." << std::endl;
        return -1;
    }

    MemBuffer certificateBuffer(bytes_received);
    certificateBuffer.write_bytes(buffer, static_cast<size_t>(bytes_received));

    try {
        std::vector<std::uint8_t> certificateBytes;
        certificateBuffer >> certificateBytes;

        Certificate certificate(certificateBytes);

        if (!certificate.verify("certs/client/ca.crt", mHost)) {
            std::cerr << "Server certificate validation failed." << std::endl;
            return -1;
        }

        std::vector<std::uint8_t> challenge(32);

        if (RAND_bytes(challenge.data(), challenge.size()) != 1) {
            std::cerr << "Failed to generate challenge." << std::endl;
            return -1;
        }

        MemBuffer challengeBuffer(32);
        challengeBuffer << challenge;

        ssize_t bytes_sent =
            send(mSocket, challengeBuffer.data(), challengeBuffer.size(), 0);

        if (bytes_sent < 0) {
            std::cerr << "Failed to send challenge." << std::endl;
            return -1;
        }

        bytes_received = recv(mSocket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            std::cerr << "Failed to receive server signature." << std::endl;
            return -1;
        }

        MemBuffer signatureBuffer(bytes_received);
        signatureBuffer.write_bytes(buffer,
                                    static_cast<size_t>(bytes_received));

        std::vector<std::uint8_t> signature;
        signatureBuffer >> signature;

        if (!certificate.verifySignature(challenge, signature)) {
            std::cerr << "Server proof of possession failed." << std::endl;
            return -1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Certificate authentication failed: " << e.what()
                  << std::endl;
        return -1;
    }
}

int NetworkHandler::performServerCertificateAuthentication() {
    std::ifstream file("certs/server/server.crt", std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open server certificate." << std::endl;
        return -1;
    }

    std::vector<std::uint8_t> certificateBytes{
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    MemBuffer certificateBuffer(1024);
    certificateBuffer << certificateBytes;

    ssize_t bytes_sent =
        send(mSocket, certificateBuffer.data(), certificateBuffer.size(), 0);

    if (bytes_sent < 0) {
        std::cerr << "Failed to send server certificate." << std::endl;
        return -1;
    }

    // Receive client challenge.
    char buffer[64 * 1024];

    ssize_t bytes_received = recv(mSocket, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0) {
        std::cerr << "Failed to receive challenge." << std::endl;
        return -1;
    }

    MemBuffer challengeBuffer(bytes_received);
    challengeBuffer.write_bytes(buffer, static_cast<size_t>(bytes_received));

    try {
        std::vector<std::uint8_t> challenge;
        challengeBuffer >> challenge;

        // Sign challenge using server private key.
        auto signature =
            Certificate::sign("certs/server/server.key", challenge);

        MemBuffer signatureBuffer(1024);
        signatureBuffer << signature;

        bytes_sent =
            send(mSocket, signatureBuffer.data(), signatureBuffer.size(), 0);

        if (bytes_sent < 0) {
            std::cerr << "Failed to send server signature." << std::endl;
            return -1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Certificate authentication failed: " << e.what()
                  << std::endl;
        return -1;
    }
}
int NetworkHandler::performClientHandshake() {
    mCryptoState = CryptoState::HANDSHAKING;

    if (performClientCertificateAuthentication() != 0) {
        return -1;
    }

    mCrypto = std::make_unique<CryptoSession>();

    auto publicKey = mCrypto->getPublicKey();

    ssize_t bytes_sent = send(mSocket, publicKey.data(), publicKey.size(), 0);

    if (bytes_sent != static_cast<ssize_t>(publicKey.size())) {
        std::cerr << "Failed to send DH public key." << std::endl;
        return -1;
    }

    std::vector<std::uint8_t> peerPublicKey(publicKey.size());

    ssize_t bytes_received =
        recv(mSocket, peerPublicKey.data(), peerPublicKey.size(), 0);

    if (bytes_received != static_cast<ssize_t>(peerPublicKey.size())) {
        std::cerr << "Failed to receive DH public key." << std::endl;
        return -1;
    }
    mCrypto->establish(peerPublicKey);
    mCryptoState = CryptoState::ESTABLISHED;

    return 0;
}

int NetworkHandler::performServerHandshake() {
    mCryptoState = CryptoState::HANDSHAKING;

    if (performServerCertificateAuthentication() != 0) {
        return -1;
    }

    mCrypto = std::make_unique<CryptoSession>();

    std::vector<std::uint8_t> peerPublicKey(256);

    ssize_t bytes_received =
        receiveBytes(peerPublicKey.data(), peerPublicKey.size());

    if (bytes_received != static_cast<ssize_t>(peerPublicKey.size())) {
        std::cerr << "Failed to receive DH public key." << std::endl;
        return -1;
    }

    auto publicKey = mCrypto->getPublicKey();

    ssize_t bytes_sent = send(mSocket, publicKey.data(), publicKey.size(), 0);

    if (bytes_sent != static_cast<ssize_t>(publicKey.size())) {
        std::cerr << "Failed to send DH public key." << std::endl;
        return -1;
    }

    mCrypto->establish(peerPublicKey);
    mCryptoState = CryptoState::ESTABLISHED;

    return 0;
}

int NetworkHandler::sendPacket(const Packet& packet) {
    if (!mConnected || mCryptoState != CryptoState::ESTABLISHED) {
        return -1;
    }

    MemBuffer buffer(1024);
    packet.serialise(buffer);

    std::vector<std::uint8_t> plaintext(buffer.data(),
                                        buffer.data() + buffer.size());

    auto encrypted = mCrypto->encrypt(plaintext);

    MemBuffer encryptedBuffer(1024);

    encryptedBuffer << encrypted.nonce;
    encryptedBuffer << encrypted.ciphertext;
    encryptedBuffer << encrypted.tag;

    ssize_t bytes_sent =
        send(mSocket, encryptedBuffer.data(), encryptedBuffer.size(), 0);

    if (bytes_sent < 0) {
        std::cerr << "Failed to send packet." << std::endl;
        return -1;
    }

    return 0;
}

int NetworkHandler::connect() {
    if (!mInitialized) {
        std::cerr << "NetworkHandler not initialized." << std::endl;
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(mPort);
    inet_pton(AF_INET, mHost.c_str(), &server_addr.sin_addr);

    if (::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        std::cerr << "Connection to " << mHost << ":" << mPort << " failed."
                  << std::endl;
        ::close(sock);
        return -1;
    }

    mConnected = true;
    mSocket = sock;

    if (performClientHandshake() != 0) {
        close();
        return -1;
    }

    return 0;
}

std::unique_ptr<Packet> NetworkHandler::receivePacket(bool noBlock) {
    if (!mConnected || mCryptoState != CryptoState::ESTABLISHED) {
        return nullptr;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(mSocket, &readfds);

    timeval timeout{};
    if (noBlock) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    } else {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }

    int select_result =
        select(mSocket + 1, &readfds, nullptr, nullptr, &timeout);

    if (select_result < 0) {
        if (errno == EINTR) {
            return nullptr;
        }

        std::cerr << "Select failed: " << std::strerror(errno) << std::endl;
        return nullptr;
    } else if (select_result == 0) {
        std::cerr << "nullptr\n";
        return nullptr;
    }

    char buffer[64 * 1024];

    ssize_t bytes_received = recv(mSocket, buffer, sizeof(buffer), 0);
    std::cerr << "[RECV] fd=" << mSocket << " bytes=" << bytes_received
              << std::endl;
    if (bytes_received == 0) {
        std::cerr << "[RECV] peer closed fd=" << mSocket << std::endl;
        close();
        return nullptr;
    }

    if (bytes_received < 0) {
        if (errno == EINTR) {
            return nullptr;
        }

        std::cerr << "Failed to receive packet: " << std::strerror(errno)
                  << std::endl;
        return nullptr;
    }

    if (bytes_received == 0) {
        close();
        return nullptr;
    }

    MemBuffer encryptedBuffer(bytes_received);

    encryptedBuffer.write_bytes(buffer, static_cast<size_t>(bytes_received));

    AESGCM::EncryptedData encrypted{};

    try {
        encryptedBuffer >> encrypted.nonce;
        encryptedBuffer >> encrypted.ciphertext;
        encryptedBuffer >> encrypted.tag;

        auto plaintext = mCrypto->decrypt(encrypted);

        MemBuffer plaintextBuffer(plaintext.size());

        plaintextBuffer.write_bytes(plaintext.data(), plaintext.size());

        return Packet::getPacketFactory(plaintextBuffer);

    } catch (const std::exception& e) {
        std::cerr << "Failed to decrypt packet: " << e.what() << std::endl;
        return nullptr;
    }
}

void NetworkHandler::close() {
    if (mSocket != -1) {
        ::close(mSocket);
        mConnected = false;
        mSocket = -1;
    }

    mCrypto.reset();
    mCryptoState = CryptoState::DISCONNECTED;
}