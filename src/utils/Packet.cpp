#include "Packet.hh"

#include <cstdint>
#include <string>

std::unique_ptr<Packet> getPacketFactory(MemBuffer& buffer) {
    PacketType packetType;
    packetType = static_cast<PacketType>(buffer.view()[0]);

    switch (packetType) {
        case PacketType::MESSAGE: {
            auto messagePacket = std::make_unique<MessagePacket>();
            messagePacket->deserialise(buffer);
            return messagePacket;
        }
        case PacketType::ERROR: {
            auto errorPacket = std::make_unique<Packet>();
            errorPacket->mPacketType = PacketType::ERROR;
            return errorPacket;
        }
        default:
            throw std::runtime_error(
                "Unknown packet type : " +
                std::to_string(static_cast<uint8_t>(packetType)));
    }
}

void MessagePacket::serialise(MemBuffer& ptr) {
    ptr << (uint8_t)mPacketType;
    ptr << mUsername;
    ptr << mMessage;
}

void MessagePacket::deserialise(MemBuffer& ptr) {
    PacketType packetType;
    ptr >> (uint8_t&)packetType;
    mPacketType = packetType;
    ptr >> mUsername;
    ptr >> mMessage;
}