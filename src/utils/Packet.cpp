#include "Packet.hh"

#include <sys/types.h>

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
        case PacketType::REQUEST: {
            auto requestPacket = std::make_unique<RequestPacket>();
            requestPacket->deserialise(buffer);
            return requestPacket;
        }
        case PacketType::FIELD_REQ: {
            auto fieldReqPacket = std::make_unique<FieldReqPacket>();
            fieldReqPacket->deserialise(buffer);
            return fieldReqPacket;
        }
        case PacketType::USER_LIST: {
            auto userListPacket = std::make_unique<UserListPacket>();
            userListPacket->deserialise(buffer);
            return userListPacket;
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

void MessagePacket::serialise(MemBuffer& ptr) const {
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

void RequestPacket::serialise(MemBuffer& ptr) const {
    ptr << (uint8_t)mPacketType;
    ptr << (uint8_t)mRequestType;
}

void RequestPacket::deserialise(MemBuffer& ptr) {
    PacketType packetType;
    ptr >> (uint8_t&)packetType;
    mPacketType = packetType;
    RequestType requestType;
    ptr >> (uint8_t&)requestType;
    mRequestType = requestType;
}

void FieldReqPacket::serialise(MemBuffer& ptr) const {
    ptr << (uint8_t)mPacketType;
    ptr << (uint8_t)mRequestType;
    ptr << mField;
}

void FieldReqPacket::deserialise(MemBuffer& ptr) {
    PacketType packetType;
    ptr >> (uint8_t&)packetType;
    mPacketType = packetType;
    RequestType requestType;
    ptr >> (uint8_t&)requestType;
    mRequestType = requestType;
    ptr >> mField;
}

void UserListPacket::serialise(MemBuffer& ptr) const {
    ptr << (uint8_t)mPacketType;
    ptr << mUserList;
}

void UserListPacket::deserialise(MemBuffer& ptr) {
    PacketType packetType;
    ptr >> (uint8_t&)packetType;
    mPacketType = packetType;
    ptr >> mUserList;
}

void ConnectionSetupPacket::serialise(MemBuffer& ptr) const {
    ptr << (uint8_t)mPacketType;
    ptr << mUsername;
}

void ConnectionSetupPacket::deserialise(MemBuffer& ptr) {
    PacketType packetType;
    ptr >> (uint8_t&)packetType;
    mPacketType = packetType;
    ptr >> mUsername;
}