#pragma once

#include <deque>
#include <map>
#include <memory>
#include <semaphore>
#include <string>
#include <string_view>

#include "utils/E2ESession.hh"
#include "utils/NetworkHandler.hh"
#include "utils/Packet.hh"

namespace client_impl {

class Client {
  private:
    NetworkHandler serverSocket;
    std::string username;
    std::string connected;
    std::deque<std::unique_ptr<Packet>> pktQueue;
    SequenceNo curSeqNo = 0;
    std::map<std::string, E2ESession> mE2ESessions;
    bool waitUntilAck(SequenceNo seq);
    std::unique_ptr<Packet> waitForType(PacketType type);

    bool initiateE2E(const std::string& username);
    void handleE2EInit(const MessagePacket& packet);
    void handleE2EAck(const MessagePacket& packet);
    void handleE2EMessage(const MessagePacket& packet);

  public:
    Client() {}
    bool setupConnection(std::string_view host, int port);
    bool login(std::string_view uname);
    bool connectTo(std::string_view uname);
    SequenceNo send(std::string_view msg);
    std::vector<std::string> getUsers();
    std::unique_ptr<Packet> poll();
    SequenceNo getSeqNo() { return curSeqNo; }
    NetworkHandler* getSocket() { return &serverSocket; }
    void quit();
};

class UI;

inline std::binary_semaphore clientBackendSem(1);

inline bool running = true;

void networkMonitor(int sockfd, std::binary_semaphore& sem);

void clientLoop(Client& client, UI& ui);
};  // namespace client_impl