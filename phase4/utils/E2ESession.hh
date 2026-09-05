#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <variant>

#include "Cryptography/AESGCM.hh"
#include "Cryptography/CryptoSession.hh"
#include "MemBuffer.h"
// will put in time and rekeying stuff later on th==in the next phase, not dong
// rn

///                                init/ack
///                        +----------------------+.
///           ./init       |           ack/.      v
/// Uninit -----------> INIT_SENT -------------> ESTABLISHED
///     |                                        ^
///     +----------------------------------------+
///                    init/ack
///
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
};

struct E2EInit {
    uint32_t seqno;
    uint64_t timestamp;
    std::vector<uint8_t> key;
};

inline void operator<<(Payload& pl, const E2EInit& init) {
    MemBuffer buf;
    buf << init.seqno;
    buf << init.timestamp;
    buf << init.key;

    // for (char i : buf.view()) {
    //     std::cerr << (unsigned int)(uint8_t)(i) << " ";
    // }
    // std::cerr << "--------\n";

    pl = Payload{Payload::Type::__E2E_INIT__, std::string(buf.view())};

    buf.free();

    // for (char i : pl.data) {
    //     std::cerr << (unsigned int)(uint8_t)(i) << " ";
    // }
}

inline void operator>>(const Payload& pl, E2EInit& init) {
    MemBuffer buf;
    buf << pl;
    uint8_t type;
    buf >> type;
    if (type != (uint8_t)Payload::Type::__E2E_INIT__)
        return;
    size_t str_size;
    buf >> str_size;
    buf >> init.seqno;
    buf >> init.timestamp;
    buf >> init.key;
}

struct E2EAck {
    uint32_t seqno;
    std::vector<uint8_t> key;
};

struct E2EMsg {
    uint32_t seqno;
    std::variant<std::string, AESGCM::EncryptedData> data;
};

inline void operator<<(Payload& pl, const E2EMsg& msg) {
    MemBuffer buf;
    buf << msg.seqno;
    if (std::holds_alternative<std::string>(msg.data)) {
        pl.type = Payload::Type::__PLAIN_TEXT__;
        buf << std::get<std::string>(msg.data);
    } else {
        pl.type = Payload::Type::__E2E_MSG__;
        auto& edata = std::get<AESGCM::EncryptedData>(msg.data);
        buf << edata.nonce;
        buf << edata.tag;
        buf << edata.ciphertext;
    }
    pl.data = std::string(buf.view());
}

inline void operator>>(const Payload& pl, E2EMsg& msg) {
    MemBuffer buf;
    buf << pl;

    uint8_t type;
    buf >> type;

    size_t str_size;
    buf >> str_size;

    buf >> msg.seqno;

    if (pl.type == Payload::Type::__PLAIN_TEXT__) {
        std::string str;
        buf >> str;
        msg.data = str;
    } else {
        AESGCM::EncryptedData edata;
        buf >> edata.nonce;
        buf >> edata.tag;
        buf >> edata.ciphertext;
        msg.data = edata;
    }
}

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
    size_t str_size;
    buf >> str_size;
    buf >> init.seqno;
    buf >> init.key;
}

class E2ESession {
  private:
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

    auto getState() const { return sessState; }

    Payload encrypt(std::string_view str);
    std::string decrypt(const Payload& pl);
};
