#include "client.h"

#include "utils/Packet.hh"

bool Client::waitUntilAck(SequenceNo seq) {
    while (true) {
        auto pkt = serverSocket.receivePacket();
        if (pkt->mPacketType == PacketType::CONTROL) {
            auto ctlPkt = getDerivedPacket<ControlPacket>(std::move(pkt));
            if (ctlPkt->getReplyTo() == seq) {
                return ctlPkt->getCtl() == ControlField::ACK ? true : false;
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

void Client::send(std::string_view msg) {
    SequenceNo seq = curSeqNo++;
    auto msgPkt = MessagePacket(connected, std::string(msg));
    msgPkt.seq = seq;
    serverSocket.sendPacket(msgPkt);
}

std::vector<std::string> Client::getUsers() {
    SequenceNo seq = curSeqNo++;
    auto pkt = RequestPacket(RequestType::REQUEST_USERLIST);
    pkt.seq = seq;
    serverSocket.sendPacket(pkt);
    auto reply = waitForType(PacketType::USER_LIST);
    auto userListPkt = getDerivedPacket<UserListPacket>(std::move(reply));
    return userListPkt->getUserList();

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
