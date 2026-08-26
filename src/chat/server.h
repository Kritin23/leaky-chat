#pragma once

#include <memory>

#include "utils/NetworkHandler.hh"
#include "utils/Packet.hh"
#include "utils/SocketSet.hh"

class Server {
  private:
    SocketSet connections;
    NetworkHandler publicSocket;

    IndexedSocketMap<std::string> usernames;

    void handleConnect();
    void handleLogin(SID, std::unique_ptr<FieldReqPacket>&&);  /// set username
    void getUserList(SID, std::unique_ptr<RequestPacket>&&);
    void handleQuit(SID, std::unique_ptr<RequestPacket>&&);

    void handleMessage(SID, std::unique_ptr<MessagePacket>&&);

  public:
    Server();
    int run();
};