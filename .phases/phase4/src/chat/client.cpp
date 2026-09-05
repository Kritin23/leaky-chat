#include "client.h"

#include <poll.h>

#include <iostream>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>

#include "UI.h"
#include "utils/Packet.hh"

namespace client_impl {
bool Client::initiateE2E(const std::string& username) {
    if (mE2ESessions.contains(username))
        return false;

    auto& session = mE2ESessions[username];
    auto publicKey = session.crypto().getPublicKey();
    std::string data(reinterpret_cast<const char*>(publicKey.data()),
                     publicKey.size());
    auto pkt =
        MessagePacket(username, Payload{Payload::Type::__E2E_INIT__, data});
    pkt.seq = curSeqNo++;
    return serverSocket.sendPacket(pkt) == 0;
}

void Client::handleE2EInit(const MessagePacket& packet) {
    const std::string& peer = packet.getSender();

    std::vector<std::uint8_t> peerPublicKey(packet.getPayload().data.begin(),
                                            packet.getPayload().data.end());
    auto& session = mE2ESessions[peer];
    session.crypto().establish(peerPublicKey);
    auto publicKey = session.crypto().getPublicKey();
    std::string data(reinterpret_cast<const char*>(publicKey.data()),
                     publicKey.size());
    auto ack = MessagePacket(peer, Payload{Payload::Type::__E2E_ACK__, data});
    ack.seq = curSeqNo++;
    serverSocket.sendPacket(ack);
}

void Client::handleE2EAck(const MessagePacket& packet) {
    const std::string& peer = packet.getSender();
    auto it = mE2ESessions.find(peer);
    if (it == mE2ESessions.end()) {
        std::cerr << "Received E2E_ACK for unknown peer: " << peer << std::endl;
        return;
    }
    std::vector<std::uint8_t> peerPublicKey(packet.getPayload().data.begin(),
                                            packet.getPayload().data.end());
    it->second.crypto().establish(peerPublicKey);
}

void Client::handleE2EMessage(const MessagePacket& packet) {
    const std::string& peer = packet.getSender();
    auto it = mE2ESessions.find(peer);
    if (it == mE2ESessions.end() || !it->second.crypto().isEstablished()) {
        std::cerr << "No E2E session with " << peer << std::endl;
        return;
    }
    const std::string& data = packet.getPayload().data;
    MemBuffer buffer(data.size());
    buffer.write_bytes(data.data(), data.size());
    AESGCM::EncryptedData encrypted{};
    buffer >> encrypted.nonce;
    buffer >> encrypted.ciphertext;
    buffer >> encrypted.tag;
    try {
        auto plaintext = it->second.crypto().decrypt(encrypted);
        std::string message(reinterpret_cast<const char*>(plaintext.data()),
                            plaintext.size());
        std::cout << peer << ": " << message << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "E2E decryption failed: " << e.what() << std::endl;
    }
}

bool Client::waitUntilAck(SequenceNo seq) {
    while (true) {
        auto pkt = serverSocket.receivePacket();
        if (!pkt) {
            continue;
        }
        if (pkt->mPacketType == PacketType::CONTROL) {
            auto ctlPkt = getDerivedPacket<ControlPacket>(std::move(pkt));
            if (ctlPkt->getReplyTo() == seq) {
                return ctlPkt->getControlField() == ControlField::ACK ? true
                                                                      : false;
            } else {
                pktQueue.push_back(std::move(pkt));
            }
        } else {
            pktQueue.push_back(std::move(pkt));
        }
    }
}

std::unique_ptr<Packet> Client::waitForType(PacketType type) {
    while (true) {
        auto pkt = serverSocket.receivePacket();
        if (!pkt) {
            return nullptr;
        }
        if (pkt->mPacketType == type) {
            return pkt;
        } else {
            pktQueue.push_back(std::move(pkt));
        }
    }
}

bool Client::login(std::string_view username) {
    SequenceNo seq = curSeqNo++;
    auto pkt = FieldReqPacket(RequestType::SET_USERNAME, std::string(username));
    pkt.seq = seq;
    serverSocket.sendPacket(pkt);
    return waitUntilAck(seq);
}

bool Client::connectTo(std::string_view uname) {
    connected = uname;
    return true;
}

SequenceNo Client::send(std::string_view msg) {
    SequenceNo seq = curSeqNo++;
    auto msgPkt = MessagePacket(connected, std::string(msg));
    msgPkt.seq = seq;
    serverSocket.sendPacket(msgPkt);
    return seq;
}

// std::vector<std::string> Client::getUsers() {
//     SequenceNo seq = curSeqNo++;
//     auto pkt = RequestPacket(RequestType::REQUEST_USERLIST);
//     pkt.seq = seq;
//     serverSocket.sendPacket(pkt);
//     auto reply = waitForType(PacketType::USER_LIST);
//     auto userListPkt = getDerivedPacket<UserListPacket>(std::move(reply));
//     return userListPkt->getUserList();

// }

std::vector<std::string> Client::getUsers() {
    SequenceNo seq = curSeqNo++;
    auto pkt = RequestPacket(RequestType::REQUEST_USERLIST);
    pkt.seq = seq;

    if (serverSocket.sendPacket(pkt) != 0) {
        return {};
    }

    auto reply = waitForType(PacketType::USER_LIST);

    if (!reply) {
        return {};
    }

    auto userListPkt = getDerivedPacket<UserListPacket>(std::move(reply));

    const auto& users = userListPkt->getUserList();

    return users;
}

std::unique_ptr<Packet> Client::poll() {
    if (pktQueue.empty()) {
        return serverSocket.receivePacket(true);
    }
    auto pkt = std::move(pktQueue.front());
    pktQueue.pop_front();
    return pkt;
}

void Client::quit() {
    SequenceNo seq;
    auto quitPkt = RequestPacket(RequestType::DISCONNECT);
    do {
        seq = curSeqNo++;
        quitPkt.seq = seq;
        serverSocket.sendPacket(quitPkt);
    } while (waitUntilAck(seq));
}

bool Client::setupConnection(std::string_view host, int port) {
    serverSocket = NetworkHandler(std::string(host), port);
    return serverSocket.connect() == 0;
}

void networkMonitor(int sockfd, std::binary_semaphore& sem) {
    while (running) {
        pollfd fd = {sockfd, POLLIN, 0};
        int num = poll(&fd, 1, 1000);
        if (num > 0) {
            sem.release();
        }
    }
}

void clientLoop(Client& client, UI& ui, std::string ServerIP, int ServerPort) {
    while (!client.setupConnection(ServerIP, ServerPort))
        ;

    std::thread netmon{[&client]() {
        networkMonitor(client.getSocket()->getFd(), clientBackendSem);
    }};

    while (running) {
        clientBackendSem.acquire();

        ClientRequest request;

        while (running && ui.tryGetRequest(request)) {
            switch (request.type) {
                case ClientRequest::Type::Login: {
                    bool success = client.login(request.username);

                    if (success) {
                        ui.editMessage(request.id, [](Message& msg) {
                            msg.text += "\n  Login Succeeded";
                        });
                    } else {
                        ui.editMessage(request.id, [](Message& msg) {
                            msg.text += "\n  Login Failed. Please retry";
                        });
                    }

                    break;
                }

                case ClientRequest::Type::SendMessage: {
                    client.connectTo(request.username);
                    SequenceNo id = client.send(request.message);
                    break;
                }

                case ClientRequest::Type::GetUsers: {
                    auto users = client.getUsers();
                    ui.editMessage(request.id, [&users](Message& msg) {
                        msg.text += "\nActive Users: \n";
                        for (auto& usr : users) {
                            msg.text += "  " + usr + "\n";
                        }
                        msg.text += "\n";
                    });
                    break;
                }

                case ClientRequest::Type::Quit:
                    client.quit();
                    running = false;
                    break;
            }
        }

        if (!running)
            break;

        while (auto pkt = client.poll()) {
            if (pkt->mPacketType == PacketType::MESSAGE) {
                auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));
                ui.addMessage(
                    Message({}, msgPkt->getSender(), msgPkt->getMessage()));
            }
        }
    }

    netmon.join();
    ui.stop();
}
}  // namespace client_impl