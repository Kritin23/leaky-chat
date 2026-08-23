#include "ConnectionSetter.hh"

#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

int ConnectionSetter::getConnections() {
    sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(mSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (clientSocket < 0) {
        std::cerr << "Failed to accept new connection" << std::endl;
        return -1;
    }
    mOwnHandler.setSocket(clientSocket);
    auto packet = mOwnHandler.receivePacket();
    if (!packet) {
        std::cerr << "Failed to receive packet from new connection" << std::endl;
        close(clientSocket);
        return -1;
    }
    if (packet->mPacketType != PacketType::CONNECTION_SETUP) {
        std::cerr << "Received unexpected packet type from new connection" << std::endl;
        close(clientSocket);
        return -1;
    }
    auto connectionSetupPacket = dynamic_cast<ConnectionSetupPacket*>(packet.get());
    if (!connectionSetupPacket) {
        std::cerr << "Failed to cast packet to ConnectionSetupPacket" << std::endl;
        close(clientSocket);
        return -1;
    }
    std::string username = connectionSetupPacket->getUsername();
    std::string clientIP = inet_ntoa(clientAddress.sin_addr);

    Connection newConnection(username, std::make_unique<NetworkHandler>(clientIP, mPort));
    newConnection.handler->setSocket(clientSocket);
    mSharedConnections->push_back(*newConnection.handler);

    std::cout << "New connection established with username: " << username << std::endl;
    return 0;
}

int ConnectionSetter::processConnections() {
    while (getConnections() == 0) {
        sleep(1);
    }
}