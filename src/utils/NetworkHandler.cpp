#include "NetworkHandler.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <cerrno>
#include <cstring>


#include <thread>
#include <chrono>
#include "MemBuffer.h"

using namespace std::chrono_literals;

NetworkHandler::~NetworkHandler() {
    close();
}

int NetworkHandler::sendPacket(const Packet& packet) {
    if (!mConnected) {
        // std::cerr << "Not connected to server." << std::endl;
        return -1;
    }

    MemBuffer buffer(1024);
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
        ::close(sock);
        return -1;
    }

    mConnected = true;
    mSocket = sock;
    return 0;
}

std::unique_ptr<Packet> NetworkHandler::receivePacket(bool noBlock) {
    if (!mConnected) {
        // std::cerr << "Not connected to server." << std::endl;
        return nullptr;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(mSocket, &readfds);

    timeval timeout{};
    if(noBlock) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    } else {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }

    std::cerr << "here\n";
    int select_result = select(mSocket + 1, &readfds, nullptr, nullptr, &timeout);

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
    std::cerr << "[RECV] fd=" << mSocket
          << " bytes=" << bytes_received << std::endl;
    if (bytes_received == 0) {
        std::cerr << "[RECV] peer closed fd=" << mSocket << std::endl;
        close();
        return nullptr;
    }

    if (bytes_received < 0) {
        if (errno == EINTR) {
            return nullptr;
        }

        std::cerr << "Failed to receive packet: " << std::strerror(errno) << std::endl;
        return nullptr;
    }

    if (bytes_received == 0) {
        close();
        return nullptr;
    }

    mRecvBuffer.write_bytes(buffer, static_cast<size_t>(bytes_received));

    return Packet::getPacketFactory(mRecvBuffer);
}

void NetworkHandler::close() {
    mConnected = false;
    if (mSocket != -1) {
        ::close(mSocket);
        mSocket = -1;
    }
}
