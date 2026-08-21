#include "Packet.hh"
#include <string>


void MessagePacket::serialise(MemBuffer& ptr) {
    ptr << (int)mPacketType;
    std::string_view usernameView(mUsername);
    std::string_view messageView(mMessage);
    ptr << usernameView;
    ptr << messageView;
}

void MessagePacket::deserialise(MemBuffer& ptr) {
    int packetTypeInt;
    ptr >> packetTypeInt;
    mPacketType = static_cast<PacketType>(packetTypeInt);
    ptr >> mUsername;
    ptr >> mMessage;
}