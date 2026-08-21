#pragma once

#include "Packet.hh"
#include "MemBuffer.h"

#include <iostream> 
#include <string>
#include <memory>

class NetworkHandler {

private:
    std::string mHost;
    int mPort;
    int mSocket = -1;
    int mInitialized = false;
    int mConnected = false;
    MemBuffer mRecvBuffer;

public:
    NetworkHandler(std::string host, int port) : mHost(host), mPort(port), mInitialized(true) {}
    NetworkHandler() : mHost("localhost"), mPort(8080), mInitialized(true) {}

    int connect();
    int sendPacket(Packet& packet);
    std::unique_ptr<Packet> receivePacket();
};