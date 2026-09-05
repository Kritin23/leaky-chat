#include "client.h"

#include <poll.h>

#include <iostream>
#include <memory>
#include <semaphore>
#include <thread>

#include "UI.h"
#include "utils/E2ESession.hh"
#include "utils/MemBuffer.h"
#include "utils/Packet.hh"

namespace client_impl {
bool Client::initiateE2E(const std::string& username) {
    auto& session = mE2ESessions[username];
    auto pkt = MessagePacket(username, *session.initiate());
    pkt.seq = curSeqNo++;
    bool res = serverSocket.sendPacket(pkt) == 0;
    return res;
}

bool Client::refreshE2E(const std::string& username) {
    auto& session = mE2ESessions[username];
    auto pkt = MessagePacket(username, *session.refresh());
    pkt.seq = curSeqNo++;
    return serverSocket.sendPacket(pkt) == 0;
}

bool Client::handleE2EInit(const MessagePacket& packet) {
    std::cerr << "handling init\n";
    const std::string& peer = packet.getSender();

    std::vector<std::uint8_t> peerPublicKey(packet.getPayload().data.begin(),
                                            packet.getPayload().data.end());
    auto& session = mE2ESessions[peer];
    auto response = session.handleInit(packet.getPayload());
    std::cerr << "handled init\n";
    if (response) {
        auto ack = MessagePacket(peer, *response);
        ack.seq = curSeqNo++;
        serverSocket.sendPacket(ack);
        std::cerr << "init response sent\n";
        return true;
    }
    return false;
}

void Client::handleE2EAck(const MessagePacket& packet) {
    std::cerr << "acking\n";
    const std::string& peer = packet.getSender();
    E2ESession& session = mE2ESessions[peer];
    session.handleAck(packet.getPayload());
    std::cerr << "acked\n";

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

std::unique_ptr<Packet> Client::waitForPred(auto pred) {
    while (true) {
        auto pkt = serverSocket.receivePacket();
        if (!pkt) {
            return nullptr;
        }
        if (pred(pkt)) {
            return pkt;
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

bool Client::setupE2E(const std::string& uname) {
    std::cerr << "hi:e2e\n";
    if (!initiateE2E(uname))
        return false;
    std::cerr << "initiated";
    auto isE2ePkt = [&uname](const std::unique_ptr<Packet>& pkt) {
        if (pkt->mPacketType != PacketType::MESSAGE)
            return false;
        auto msg = (MessagePacket*)(pkt.get());
        auto pl = msg->getPayload();
        if ((pl.type == Payload::Type::__E2E_ACK__ ||
             pl.type == Payload::Type::__E2E_INIT__) &&
            msg->getSender() == uname) {
            return true;
        } else {
            return false;
        }
    };
    std::cerr << "here1\n";
    auto pkt = waitForPred(isE2ePkt);
    std::cerr << "pk1 rcvd\n";
    auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));
    std::cerr << "downcast\n";
    if (msgPkt->getPayload().type == Payload::Type::__E2E_INIT__) {
        if (handleE2EInit(*msgPkt)) {
            return true;
        } else {
            auto ackPkt = waitForPred(isE2ePkt);
            auto ackMsgPkt = getDerivedPacket<MessagePacket>(std::move(ackPkt));
            handleE2EAck(*ackMsgPkt);
            return true;
        }
    } else if (msgPkt->getPayload().type == Payload::Type::__E2E_ACK__) {
        handleE2EAck(*msgPkt);
        return true;
    }
    std::cerr << "bye\n";
    return false;
}

void Client::handleE2EPacket(const std::unique_ptr<MessagePacket>& pkt) {
    Payload pl = pkt->getPayload();
    if (pl.type == Payload::Type::__E2E_INIT__) {
        handleE2EInit(*pkt);
    } else if (pl.type == Payload::Type::__E2E_ACK__) {
        handleE2EAck(*pkt);
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
    if (mE2ESessions.contains(connected)) {
        auto session = mE2ESessions[connected];
        if (session.isStale())
            refreshE2E(std::string(uname));
    }
    return true;
}

SequenceNo Client::send(std::string_view msg) {
    std::cerr << "here:send\n";
    SequenceNo seq = curSeqNo++;
    auto& session = mE2ESessions[connected];
    if (session.isStale()) {
        refreshE2E(std::string(connected));
    }
    auto msgPkt = MessagePacket(connected, session.encrypt(msg));
    msgPkt.seq = seq;
    serverSocket.sendPacket(msgPkt);
    std::cerr << "byeee:send\n";
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

std::string Client::decryptMessage(const std::unique_ptr<MessagePacket>& pkt) {
    const std::string peer = pkt->getSender();
    E2ESession& session = mE2ESessions[peer];
    return session.decrypt(pkt->getPayload());
}

void Client::quit() {
    SequenceNo seq;
    auto quitPkt = RequestPacket(RequestType::DISCONNECT);
    seq = curSeqNo++;
    quitPkt.seq = seq;
    serverSocket.sendPacket(quitPkt);
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

void clientLoop(Client& client, UI& ui, std::string host, int port) {
    while (!client.setupConnection(host, port))
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
                            msg.text +=
                                "\n  \033[31mLogin Failed. Please retry\033[0m";
                        });
                    }

                    break;
                }

                case ClientRequest::Type::SendMessage: {
                    client.connectTo(request.username);
                    SequenceNo id = client.send(request.message);
                    std::cout << "sent\n";
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

                case ClientRequest::Type::E2E: {
                    bool success = client.setupE2E(request.username);
                    if (success) {
                        ui.editMessage(request.id, [](Message& msg) {
                            msg.text += "\n E2E Connection Succeeded";
                        });
                    } else {
                        ui.editMessage(request.id, [](Message& msg) {
                            msg.text += "\n E2E Connection Failed";
                        });
                    }

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
                if (msgPkt->getPayload().isE2EControl()) {
                    client.handleE2EPacket(msgPkt);
                } else {
                    std::string msg = client.decryptMessage(msgPkt);
                    ui.addMessage(Message({}, msgPkt->getSender(), msg));
                }
            }
        }

        if(!client.getSocket()->connected()) {
            ui.addMessage(Message({}, "","Server desconnected"));
            running = false;
            break;
        }
    }

    netmon.join();
}
}  // namespace client_impl