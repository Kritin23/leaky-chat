#include "NetworkHandler.hh"
#include "MemBuffer.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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

    if (::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection to " << mHost << ":" << mPort << " failed." << std::endl;
        close(sock);
        return -1;
    }

    mConnected = true;
    mSocket = sock;
    return 0;
}

int NetworkHandler::sendPacket(Packet& packet) {
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

Packet getPacket(PacketType packetType, MemBuffer& buffer) {
    Packet packet;
    switch (packetType) {
        case PacketType::MESSAGE:
            // we will iclude a class, call it MessagePacket
            // MessagePacket messagePacket; 
            // messagePacket.deserialise(buffer);
            // packet = messagePacket;
            break;
        default:
            std::cerr << "Unknown packet type received." << std::endl;
            break;
    }
    return packet;
}

Packet NetworkHandler::receivePacket() {
    if (!mConnected) {
        std::cerr << "Not connected to server." << std::endl;
        return Packet();
    }

    ssize_t bytes_received = recv(mSocket, mRecvBuffer.data(), mRecvBuffer.capacity(), 0);
    if (bytes_received < 0) {
        std::cerr << "Failed to receive packet." << std::endl;
        return Packet();
    }
    PacketType packetType;
    packetType = static_cast<PacketType>(mRecvBuffer.view()[0]);
    Packet packet = getPacket(packetType, mRecvBuffer);
    return packet;
}

