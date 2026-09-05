#pragma once

#include <chrono>
#include <optional>
#include <thread>

#include "Cryptography/CryptoSession.hh"
#include "MemBuffer.h"
// will put in time and rekeying stuff later on th==in the next phase, not dong
// rn

///                                init/ack            init/ack
///                        +----------------------+.  +----+
///           ./init       |           ack/.      v   \    v
/// Uninit -----------> INIT_SENT -------------> ESTABLISHED<------+
///     |                                        ^    |            |
///     +----------------------------------------+    | ./init     |init/ack
///                    init/ack                       v            |
///                                                 REFRESH_SENT --+
///
/// E2E Packets:
///
///    __E2E_INIT__:
///        seqno (uint32_t)
///        timestamp (uint64_t)
///        key (vector<uint8_t>)
///
///    __E2E_ACK__:
///        seqno (uint32_t)
///        key (vector<uint8_t>)

enum class E2EState {
    UNITIALIZED,
    INIT_SENT,
    ESTABLISHED,
    REFRESH_SENT,
};

struct E2EInit {
    uint32_t seqno;
    uint64_t timestamp;
    std::vector<uint8_t> key;
};

inline void operator<<(Payload& pl, const E2EInit& init) {
    MemBuffer buf;
    buf << init.seqno << init.timestamp << init.key;
    pl = Payload{Payload::Type::__E2E_INIT__,
                 std::string(buf.data(), buf.size())};
}

inline void operator>>(const Payload& pl, E2EInit& init) {
    MemBuffer buf;
    buf << pl;
    uint8_t type;
    buf >> type;
    if (type != (uint8_t)Payload::Type::__E2E_INIT__)
        return;
    buf >> init.seqno;
    buf >> init.timestamp;
    buf >> init.key;
}

struct E2EAck {
    uint32_t seqno;
    std::vector<uint8_t> key;
};

inline void operator<<(Payload& pl, const E2EAck& init) {
    MemBuffer buf;
    buf << init.seqno << init.key;
    pl = Payload{Payload::Type::__E2E_ACK__,
                 std::string(buf.data(), buf.size())};
}

inline void operator>>(const Payload& pl, E2EAck& init) {
    MemBuffer buf;
    buf << pl;
    uint8_t type;
    buf >> type;
    if (type != (uint8_t)Payload::Type::__E2E_ACK__)
        return;
    buf >> init.seqno;
    buf >> init.key;
}

class E2ESession {
  private:
    CryptoSession oldCrypto;
    CryptoSession mCrypto;
    uint64_t sessTimestamp = -1;
    E2EState sessState = E2EState::UNITIALIZED;
    uint32_t sequenceNo = 0;

  public:
    E2ESession() = default;

    CryptoSession& crypto() { return mCrypto; }

    const CryptoSession& crypto() const { return mCrypto; }

    uint32_t seqNo() const { return sequenceNo; }

    std::optional<Payload> initiate();
    std::optional<Payload> handleInit(Payload pl);
    std::optional<Payload> handleAck(Payload pl);

    Payload encrypt(std::string_view str);
    std::string decrypt(const Payload& pl);

};
