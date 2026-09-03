#pragma once

#include "Cryptography/CryptoSession.hh"
#include "MemBuffer.h"

#include <optional>
// will put in time and rekeying stuff later on th==in the next phase, not dong rn


///                                init/ack
///                        +----------------------+
///           ./init       |           ack/.      v
/// Uninit -----------> INIT_SENT -------------> ESTABLISHED
///     |                                        ^ 
///     +----------------------------------------+ 
///                    init/ack
enum class E2EState {
    UNITIALIZED,
    INIT_SENT,
    ESTABLISHED
};

class E2ESession {
  private:
    CryptoSession mCrypto;
    E2EState sessState = E2EState::UNITIALIZED;
    uint64_t sessTimestamp = -1;

  public:
    E2ESession() = default;

    CryptoSession& crypto() {
        return mCrypto;
    }

    const CryptoSession& crypto() const {
        return mCrypto;
    }

    std::optional<Payload> initiate();
    std::optional<Payload> handleInit(Payload pl);
    std::optional<Payload> handleAck(Payload pl); 

    Payload encrypt(std::string_view str);
    std::string decrypt(const Payload& pl);

};
