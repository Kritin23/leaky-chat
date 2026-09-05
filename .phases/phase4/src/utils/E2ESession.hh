#pragma once

#include "Cryptography/CryptoSession.hh"
// will put in time and rekeying stuff later on th==in the next phase, not dong rn
class E2ESession {
  private:
    CryptoSession mCrypto;

  public:
    E2ESession() = default;

    CryptoSession& crypto() {
        return mCrypto;
    }

    const CryptoSession& crypto() const {
        return mCrypto;
    }
};