#pragma once

#include <sys/types.h>

#include <cstdint>
#include <memory>
#include <type_traits>

#include "MemBuffer.h"

enum class PacketType : uint8_t {
    MESSAGE,
    ERROR,
    REQUEST,
    FIELD_REQ,
    USER_LIST,
    CONNECTION_SETUP,
    CONTROL,
};

enum class RequestType : uint8_t {
    REQUEST_USERLIST,
    SET_USERNAME,
    DISCONNECT,
    ERROR,
};

enum class ControlField : uint8_t {
    ACK,
    NACK,
    ERROR,
};

using SequenceNo = uint32_t;

class Packet {
  public:
    PacketType mPacketType;
    SequenceNo seq;

    static std::unique_ptr<Packet> getPacketFactory(MemBuffer& buffer);

    // these take MemBuffer instead of void* because we don't want the data and
    // MemBuffer state(begin,end,capacity) to be in need of being updated
    // separately
    virtual void serialise(MemBuffer& ptr) const {
        ptr << (uint8_t)mPacketType;
        ptr << seq;
    };  // must use write_bytes interface of MemBuffer

    virtual void deserialise(MemBuffer& ptr) {
        ptr >> (uint8_t&)mPacketType;
        ptr >> seq;
    }
    virtual ~Packet() = default;
};

template <typename T>
    requires std::is_base_of_v<Packet, T>
std::unique_ptr<T> getDerivedPacket(std::unique_ptr<Packet>&& pkt) {
    T* der = static_cast<T*>(pkt.release());
    return std::unique_ptr<T>(der);
}

class MessagePacket : public Packet {
    std::string mSender;
    std::string mReceiver;
    std::string mMessage;

  public:
    MessagePacket(const std::string& sender,
                  const std::string receiver,
                  const std::string& message)
        : mSender(sender), mReceiver(receiver), mMessage(message) {}

    MessagePacket(const std::string& username, const std::string& message)
        : MessagePacket("", username, message) {
        mPacketType = PacketType::MESSAGE;
    }
    MessagePacket() { mPacketType = PacketType::MESSAGE; }
    void serialise(MemBuffer& ptr) const override;
    void deserialise(MemBuffer& ptr) override;
    const std::string& getSender() const { return mSender; }
    const std::string& getReceiver() const { return mReceiver; }
    const std::string& getMessage() const { return mMessage; }

    void setSender(const std::string& uname) { mSender = uname; }
};

class RequestPacket : public Packet {
    RequestType mRequestType;

  public:
    RequestPacket(RequestType requestType) : mRequestType(requestType) {
        mPacketType = PacketType::REQUEST;
    }
    RequestPacket() { mPacketType = PacketType::REQUEST; }
    void serialise(MemBuffer& ptr) const override;
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
    void serialise(MemBuffer& ptr) const override;
    void deserialise(MemBuffer& ptr) override;
    std::string getField() const { return mField; }
};

class UserListPacket : public Packet {
    std::vector<std::string> mUserList;

  public:
    UserListPacket(const std::vector<std::string>& userList)
        : mUserList(userList) {
        mPacketType = PacketType::USER_LIST;
    }
    UserListPacket(std::vector<std::string>&& userList)
        : mUserList(std::move(userList)) {
        mPacketType = PacketType::USER_LIST;
    }

    UserListPacket() { mPacketType = PacketType::USER_LIST; }
    void serialise(MemBuffer& ptr) const override;
    void deserialise(MemBuffer& ptr) override;
    const std::vector<std::string>& getUserList() const { return mUserList; }
};

class ConnectionSetupPacket : public Packet {
    std::string mUsername;

  public:
    ConnectionSetupPacket(const std::string& username) : mUsername(username) {
        mPacketType = PacketType::CONNECTION_SETUP;
    }
    ConnectionSetupPacket() { mPacketType = PacketType::CONNECTION_SETUP; }
    void serialise(MemBuffer& ptr) const override;
    void deserialise(MemBuffer& ptr) override;
    const std::string& getUsername() const { return mUsername; }
};

class ControlPacket : public Packet {
    ControlField mControl;
    SequenceNo mReplyTo;

  public:
    ControlPacket(ControlField fld, SequenceNo reply)
        : mControl(fld), mReplyTo(reply) {
        mPacketType = PacketType::CONTROL;
    }

    ControlPacket() { mPacketType = PacketType::CONTROL; }

    void serialise(MemBuffer& ptr) const override;
    void deserialise(MemBuffer& ptr) override;

    void setControlField(ControlField fld) { mControl = fld; }
    ControlField getControlField() const { return mControl; }
    SequenceNo getReplyTo() const { return mReplyTo; }
};
