#pragma once

#include <cstdint>
#include <string>

#include "utils/NetworkHandler.hh"

class MalloryProxy {
  private:
    std::string mServerHost;
    uint16_t mServerPort;
    uint16_t mListenPort;
    int mListenSocket = -1;
    bool mTamperClientToServer = false;

    NetworkHandler mClientSide;
    NetworkHandler mServerSide;

    int createListenSocket();
    void relayClientToServer();
    void relayServerToClient();
    void inspectPacket(const char* direction, const Packet& packet);

  public:
    MalloryProxy(const std::string& serverHost,
                 uint16_t serverPort,
                 uint16_t listenPort,
                 bool tamperClientToServer);
    ~MalloryProxy();

    int run();
};