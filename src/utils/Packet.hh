#pragma once

#include <sys/types.h>

#include <cstdint>
#include <memory>

#include "MemBuffer.h"

enum class PacketType : uint8_t {
    MESSAGE,
    ERROR,
    REQUEST,
    FIELD_REQ,
    USER_LIST
};

enum class RequestType : uint8_t {
    ERROR,
};

class Packet {
  public:
    PacketType mPacketType;

    static std::unique_ptr<Packet> getPacketFactory(MemBuffer& buffer);
    // these take MemBuffer instead of void* because we don't want the data and
    // MemBuffer state(begin,end,capacity) to be in need of being updated
    // separately
    virtual void serialise(MemBuffer& ptr) {
    };  // must use write_bytes interface of MemBuffer
    virtual void deserialise(MemBuffer& ptr);
    virtual ~Packet() = default;
};

class MessagePacket : public Packet {
    std::string mUsername;
    std::string mMessage;

  public:
    MessagePacket(const std::string& username, const std::string& message)
        : mUsername(username), mMessage(message) {
        mPacketType = PacketType::MESSAGE;
    }
    MessagePacket() { mPacketType = PacketType::MESSAGE; }
    void serialise(MemBuffer& ptr) override;
    void deserialise(MemBuffer& ptr) override;
    std::string getUsername() const { return mUsername; }
    std::string getMessage() const { return mMessage; }
};

class RequestPacket : public Packet {
    RequestType mRequestType;

  public:
    RequestPacket(RequestType requestType) : mRequestType(requestType) {
        mPacketType = PacketType::REQUEST;
    }
    RequestPacket() { mPacketType = PacketType::REQUEST; }
    void serialise(MemBuffer& ptr) override;
    void deserialise(MemBuffer& ptr) override;
};

class FieldReqPacket : public Packet {
    RequestType mRequestType;
    std::string mField;

  public:
    FieldReqPacket(RequestType requestType, const std::string& field)
        : mRequestType(requestType), mField(field) {
        mPacketType = PacketType::FIELD_REQ;
    }
    FieldReqPacket() { mPacketType = PacketType::FIELD_REQ; }
    void serialise(MemBuffer& ptr) override;
    void deserialise(MemBuffer& ptr) override;
};

class UserListPacket : public Packet {
    std::vector<std::string> mUserList;

  public:
    UserListPacket(const std::vector<std::string>& userList)
        : mUserList(userList) {
        mPacketType = PacketType::USER_LIST;
    }
    UserListPacket() { mPacketType = PacketType::USER_LIST; }
    void serialise(MemBuffer& ptr) override;
    void deserialise(MemBuffer& ptr) override;
    const std::vector<std::string>& getUserList() const { return mUserList; }
};
