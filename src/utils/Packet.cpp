#include "Packet.hh"

#include <sys/types.h>

#include <cstdint>
#include <string>
#include "utils/MemBuffer.h"

std::unique_ptr<Packet> Packet::getPacketFactory(MemBuffer& buffer) {
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
        case PacketType::CONNECTION_SETUP: {
            auto connectionSetupPacket = std::make_unique<ConnectionSetupPacket>();
            connectionSetupPacket->deserialise(buffer);
            return connectionSetupPacket;
        }
        case PacketType::CONTROL: {
            auto controlPacket = std::make_unique<ControlPacket>();
            controlPacket->deserialise(buffer);
            return controlPacket;
        }
        default:
            throw std::runtime_error(
                "Unknown packet type : " +
                std::to_string(static_cast<uint8_t>(packetType)));
    }
}

void MessagePacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
    ptr << mSender;
    ptr << mReceiver;
    ptr << mPayload;
}

void MessagePacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
    ptr >> mSender;
    ptr >> mReceiver;
    ptr >> mPayload;
}

void RequestPacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
    ptr << (uint8_t)mRequestType;
}

void RequestPacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
    RequestType requestType;
    ptr >> (uint8_t&)requestType;
    mRequestType = requestType;
}

void FieldReqPacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
    ptr << (uint8_t)mRequestType;
    ptr << mField;
}

void FieldReqPacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
    RequestType requestType;
    ptr >> (uint8_t&)requestType;
    mRequestType = requestType;
    ptr >> mField;
}

void UserListPacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
    ptr << mUserList;
}

void UserListPacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
    ptr >> mUserList;
}

void ConnectionSetupPacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
}

void ConnectionSetupPacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
}

void ControlPacket::serialise(MemBuffer& ptr) const {
    this->Packet::serialise(ptr);
    ptr << (uint8_t) mControl;
    ptr << (uint32_t) mReplyTo;
}

void ControlPacket::deserialise(MemBuffer& ptr) {
    this->Packet::deserialise(ptr);
    ptr >> (uint8_t&) mControl;
    ptr >> (uint32_t&) mReplyTo;
}