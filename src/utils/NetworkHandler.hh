#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "MemBuffer.h"
#include "Packet.hh"

class NetworkHandler {
  private:
    std::string mHost;
    int mPort;
    int mSocket = -1;
    int mInitialized = false;
    int mConnected = false;
    MemBuffer mRecvBuffer;

  public:
    NetworkHandler(std::string host, int port)
        : mHost(host), mPort(port), mInitialized(true) {}
    NetworkHandler() : mHost("localhost"), mPort(8080), mInitialized(true) {}

    ~NetworkHandler();

    int connect();
    int sendPacket(std::unique_ptr<Packet>& packet);
    std::unique_ptr<Packet> receivePacket();
    int getSocket() const { return mSocket; }
    int setSocket(int socket) {
        mSocket = socket;
        mConnected = true;
        return 0;
    }
};