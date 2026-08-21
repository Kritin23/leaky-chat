#include "MemBuffer.h"
#include <cstdint>

enum class PacketType : int {
    MESSAGE,
};

class Packet {
public:
    PacketType mPacketType;
    virtual void serialise(MemBuffer& ptr); // must use write_bytes interface of MemBuffer
    virtual void deserialise(MemBuffer& ptr);
};