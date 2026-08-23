// this class shall handle new connections, make networkHandlers for them, and put them in some sort of cross thread shared queue or similar for the looping thread to handle other stuff for now, later we can see if putting both logic in the same thread makes more sense
#pragma once

#include "NetworkHandler.hh"
#include "SharedVector.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

struct Connection {
    std::string username;
    std::unique_ptr<NetworkHandler> handler;

    Connection(
        std::string username,
        std::unique_ptr<NetworkHandler> handler)
        : username(std::move(username)),
          handler(std::move(handler)) {}
};

class ConnectionSetter {
  private:
    uint16_t mPort;
    int mSocket = -1;
    sockaddr_in mAddress;
    NetworkHandler mOwnHandler;
    std::unique_ptr<SharedVector<NetworkHandler>> mSharedConnections;
    bool mRunning = false;


  public:
    ConnectionSetter(uint16_t port)
        : mPort(port),
          mOwnHandler("127.0.0.1", port),
          mSharedConnections(std::make_unique<SharedVector<NetworkHandler>>()) {

            mSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (mSocket < 0) {
                throw std::runtime_error("Failed to create socket");
            }
            fcntl(mSocket, F_SETFL, O_NONBLOCK);
            mOwnHandler.setSocket(mSocket);

            mAddress.sin_family = AF_INET;
            mAddress.sin_addr.s_addr = INADDR_ANY;
            mAddress.sin_port = htons(mPort);

            if (::bind(mSocket, (struct sockaddr*)&mAddress, sizeof(mAddress)) < 0) {
                throw std::runtime_error("Failed to bind socket");
            }

          }

    ~ConnectionSetter();

    int getConnections();

    int processConnections();



};