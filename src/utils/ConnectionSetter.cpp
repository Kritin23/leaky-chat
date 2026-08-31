#include "ConnectionSetter.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

int ConnectionSetter::getConnections() {
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientSocket =
        accept(mSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);

    if (clientSocket < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }

        std::cerr << "Failed to accept new connection: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }

    std::string clientIP = inet_ntoa(clientAddress.sin_addr);

    NetworkHandler handler(clientIP, mPort);
    handler.setSocket(clientSocket);

    if (handler.performServerHandshake() != 0) {
        handler.close();
        return -1;
    }

    SID sid = mConnections.insert(std::move(handler));

    std::cerr << "New connection established with: " << clientIP << ":"
              << ntohs(clientAddress.sin_port) << " SID=" << sid << std::endl;

    return 0;
}

int ConnectionSetter::processConnections() {
    // clear the shared connections vector
    // mSharedConnections = std::make_unique<SharedVector<NetworkHandler>>();
    while (getConnections() == 0) {
        // sleep(1);
        // std::cerr << "Waiting for new connections..." << std::endl;
    }
    return 0;
}

ConnectionSetter::~ConnectionSetter() {
    if (mSocket >= 0) {
        close(mSocket);
    }
}