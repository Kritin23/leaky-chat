#include "server.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include "utils/NetworkHandler.hh"
#include "utils/Packet.hh"

void Server::handleConnect() {
    connectionSetter.processConnections();
}

void Server::handleLogin(SID sid, std::unique_ptr<FieldReqPacket>&& pkt) {
    std::string requestedUname = pkt->getField();
    ControlPacket reply(ControlField::ERROR, pkt->seq);
    if (usernames.contains(requestedUname)) {
        reply.setControlField(ControlField::NACK);
    } else {
        reply.setControlField(ControlField::ACK);
        usernames.insert(sid, requestedUname);
    }
    connections[sid].sendPacket(reply);
}

void Server::getUserList(SID sid, std::unique_ptr<RequestPacket>&& pkt) {
    UserListPacket reply(usernames.data());
    connections[sid].sendPacket(reply);
}

void Server::handleQuit(SID sid, std::unique_ptr<RequestPacket>&& pkt) {
    ControlPacket ctlPkt(ControlField::ACK, pkt->seq);
    connections[sid].sendPacket(ctlPkt);
    connections[sid].close();
}

void dumpMessage(const MessagePacket& pkt) {
    std::cerr << pkt.getSender() << " -> " << pkt.getReceiver() << ": ";
    std::cerr << pkt.getPayload().data << "\n";
}

void Server::handleMessage(SID sid, std::unique_ptr<MessagePacket>&& pkt) {
    const std::string& sender = usernames[sid];
    pkt->setSender(sender);
    auto rcvrOpt = usernames.getSID(pkt->getReceiver());
    if (!rcvrOpt)
        return;
    SID rcvr = *rcvrOpt;
    dumpMessage(*pkt);
    connections[rcvr].sendPacket(*pkt);
}

int Server::run() {
    while (true) {
        // std::cerr << "Waiting for new connections..." << std::endl;
        handleConnect();
        if (!connections.hasActive()) {
            std::this_thread::sleep_for(10ms);
            continue;
        }
        SID sid = connections.waitForRead();

        if (sid == -1) {
            continue;
        }
        std::cerr << "Received packet from connection " << sid
                  << std::endl;

        NetworkHandler* handler = &connections[sid];

        auto pkt = handler->receivePacket();

        if (!pkt) {
            std::cerr << "Failed to receive packet from connection " << sid
                      << '\n';
            continue;
        }
        std::cerr << "Received packet of type "
                  << static_cast<int>(pkt->mPacketType) << " from connection "
                  << sid << '\n';

        switch (pkt->mPacketType) {
            case PacketType::FIELD_REQ: {
                auto fieldReqPkt =
                    getDerivedPacket<FieldReqPacket>(std::move(pkt));

                handleLogin(sid, std::move(fieldReqPkt));

                break;
            }

            case PacketType::REQUEST: {
                auto reqPkt = getDerivedPacket<RequestPacket>(std::move(pkt));

                if (reqPkt->mRequestType == RequestType::DISCONNECT) {
                    handleQuit(sid, std::move(reqPkt));
                } else {
                    getUserList(sid, std::move(reqPkt));
                }

                break;
            }

            case PacketType::MESSAGE: {
                auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));

                handleMessage(sid, std::move(msgPkt));

                break;
            }

            default:
                std::cerr << "Unexpected packet type from " << sid << '\n';
                break;
        }
    }
}