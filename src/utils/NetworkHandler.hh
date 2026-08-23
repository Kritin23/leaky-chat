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

    int connect();
    int getFd() const { return mSocket; }
    int sendPacket(const Packet& packet);
    std::unique_ptr<Packet> receivePacket();
};