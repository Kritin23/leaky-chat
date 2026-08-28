#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "MemBuffer.h"
#include "Packet.hh"

static constexpr size_t RECV_BUFFER_SIZE = 64 * 1024;

class NetworkHandler {
  private:
    std::string mHost;
    int mPort;
    int mSocket = -1;
    int mInitialized = false;
    int mConnected = false;
    MemBuffer mRecvBuffer{RECV_BUFFER_SIZE};

  public:
    NetworkHandler(std::string host, int port)
        : mHost(host), mPort(port), mInitialized(true) {}
    NetworkHandler() : mHost("localhost"), mPort(10101), mInitialized(true) {}

    NetworkHandler(const NetworkHandler&) = delete;
    NetworkHandler& operator=(const NetworkHandler&) = delete;

    // MOVE SOCKET OWNERSHIP
    NetworkHandler(NetworkHandler&& other) noexcept
        : mHost(std::move(other.mHost)),
          mPort(other.mPort),
          mSocket(other.mSocket),
          mInitialized(other.mInitialized),
          mConnected(other.mConnected),
          mRecvBuffer(std::move(other.mRecvBuffer)) {

        other.mSocket = -1;
        other.mConnected = false;
    }

    NetworkHandler& operator=(NetworkHandler&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        close();

        mHost = std::move(other.mHost);
        mPort = other.mPort;
        mSocket = other.mSocket;
        mInitialized = other.mInitialized;
        mConnected = other.mConnected;
        mRecvBuffer = std::move(other.mRecvBuffer);

        other.mSocket = -1;
        other.mConnected = false;

        return *this;
    }

    ~NetworkHandler();

    int connect();
    int sendPacket(const Packet& packet);
    std::unique_ptr<Packet> receivePacket();
    int getSocket() const { return mSocket; }
    int setSocket(int socket) {
        mSocket = socket;
        mConnected = true;
        return 0;
    }
    int getFd() const { return mSocket; }
    bool connected() const {return mConnected;}
    void close();
};