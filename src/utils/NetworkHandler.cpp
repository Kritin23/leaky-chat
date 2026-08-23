#include "NetworkHandler.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "MemBuffer.h"

int NetworkHandler::sendPacket(const Packet& packet) {
    if (!mConnected) {
        std::cerr << "Not connected to server." << std::endl;
        return -1;
    }

    MemBuffer buffer;
    packet.serialise(buffer);

    ssize_t bytes_sent = send(mSocket, buffer.data(), buffer.size(), 0);
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
        close(sock);
        return -1;
    }

    mConnected = true;
    mSocket = sock;
    return 0;
}

std::unique_ptr<Packet> NetworkHandler::receivePacket() {
    if (!mConnected) {
        std::cerr << "Not connected to server." << std::endl;
        return std::make_unique<Packet>();
    }

    ssize_t bytes_received =
        recv(mSocket, mRecvBuffer.data(), mRecvBuffer.capacity(), 0);
    if (bytes_received < 0) {
        std::cerr << "Failed to receive packet." << std::endl;
        return std::make_unique<Packet>();
    }
    return Packet::getPacketFactory(mRecvBuffer);
}
