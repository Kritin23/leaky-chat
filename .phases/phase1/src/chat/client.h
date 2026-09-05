#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <string_view>

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
    Client() {}
    bool setupConnection(std::string_view host, int port);
    bool login(std::string_view uname);
    bool connectTo(std::string_view uname);
    SequenceNo send(std::string_view msg);
    std::vector<std::string> getUsers();
    std::unique_ptr<Packet> poll();
    SequenceNo getSeqNo() { return curSeqNo; }
    NetworkHandler* getSocket() {return &serverSocket;}
    void quit();
};

class UI;

namespace client {
inline std::binary_semaphore clientBackendSem(1);

inline bool running = true;

void networkMonitor(int sockfd, std::binary_semaphore& sem);

void clientLoop(Client& client, UI& ui, std::string host, int port);
};  // namespace client