#pragma once

#include <sys/types.h>

#include <cstdint>
#include <memory>

#include "MemBuffer.h"

enum class PacketType : uint8_t { MESSAGE, ERROR };

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
