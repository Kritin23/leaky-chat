#pragma once

#include <sys/types.h>
#include <memory>

#include "utils/NetworkHandler.hh"
#include "utils/Packet.hh"
#include "utils/SocketSet.hh"
#include "utils/ConnectionSetter.hh"

class Server {
  uint16_t mPort = 10101;
  private:
    SocketSet connections;
    NetworkHandler publicSocket;
    ConnectionSetter connectionSetter{mPort, connections};

    IndexedSocketMap<std::string> usernames;

    void handleConnect();
    void handleLogin(SID, std::unique_ptr<FieldReqPacket>&&);  /// set username
    void getUserList(SID, std::unique_ptr<RequestPacket>&&);
    void handleQuit(SID, std::unique_ptr<RequestPacket>&&);

    void handleMessage(SID, std::unique_ptr<MessagePacket>&&);

  public:
    Server() : mPort(10101),
      connectionSetter(mPort, connections),
      usernames(connections) {} 
    Server(uint16_t port) : mPort(port), publicSocket("0.0.0", mPort), usernames(connections) {
        publicSocket.connect();
    }
    int run();
};