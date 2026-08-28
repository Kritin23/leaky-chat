#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <deque>

#include "utils/NetworkHandler.hh"
#include "utils/Packet.hh"

class Client {
  private:
    NetworkHandler serverSocket;
    std::string username;
    std::string connected;
    std::deque<std::unique_ptr<Packet>> pktQueue;
    SequenceNo curSeqNo = 0;
    bool waitUntilAck(SequenceNo seq);
    std::unique_ptr<Packet> waitForType(PacketType type);

  public:
    Client() {

    }
    bool setupConnection(std::string_view host, int port);
    bool login(std::string_view uname);  
    bool connectTo(std::string_view uname);
    void send(std::string_view msg);
    std::vector<std::string> getUsers();
    auto poll();
    void quit();
};