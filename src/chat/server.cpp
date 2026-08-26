#include "server.h"
#include <memory>
#include "utils/Packet.hh"

void Server::handleLogin(SID sid, std::unique_ptr<FieldReqPacket>&& pkt) {
    std::string requestedUname = pkt->getField();
    ControlPacket reply(ControlField::ERROR, pkt->seq);
    if(usernames.contains(requestedUname)) {
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

void Server::handleMessage(SID sid, std::unique_ptr<MessagePacket>&& pkt) {
    const std::string& sender = usernames[sid];
    pkt->setSender(sender);
    auto rcvrOpt = usernames.getSID(pkt->getReceiver());
    if (!rcvrOpt)
        return;
    SID rcvr = *rcvrOpt;
    connections[rcvr].sendPacket(*pkt);


}
