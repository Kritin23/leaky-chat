#include "E2ESession.hh"

#include <openssl/bn.h>
#include <openssl/rand.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

#include "utils/Cryptography/AESGCM.hh"
#include "utils/MemBuffer.h"

std::optional<Payload> E2ESession::initiate() {
    std::vector<uint8_t> publicKey = mCrypto.getPublicKey();
    uint64_t timestamp =
        std::chrono::system_clock::now().time_since_epoch().count();
    MemBuffer buf;
    buf << timestamp;

    for (char i : buf.view()) {
        std::cerr << (unsigned int)((uint8_t)i) << " ";
    }

    buf << publicKey;

    std::cerr << "Timestamp: " << timestamp << "\n";

    for (char i : buf.view()) {
        std::cerr << (unsigned int)((uint8_t)i) << " ";
    }
    std::cerr << "\n";

    Payload pl{Payload::Type::__E2E_INIT__, std::string(buf.view())};
    sessState = E2EState::INIT_SENT;
    sessTimestamp = timestamp;
    return pl;
}

std::optional<Payload> E2ESession::handleInit(Payload pl) {
    std::cerr << "e2e:handleinit\n";
    MemBuffer buf;
    buf << pl.data;

    for (char i : buf.view()) {
        std::cerr << (unsigned int)((uint8_t)i) << " ";
    }

    uint64_t timestamp;
    buf >> timestamp;

    std::cerr << "-------------\n";

    for (char i : buf.view()) {
        std::cerr << (unsigned int)((uint8_t)i) << " ";
    }

    if (timestamp < sessTimestamp) {
        std::cerr << "here\n";
        std::cerr << "\n";
        std::vector<uint8_t> peerKey;
        buf >> peerKey;

        std::cerr << "peer key: ";
        for (auto i : peerKey) {
            std::cerr << i << "\n";
        }
        mCrypto.establish(peerKey);

        std::cerr << "est\n";

        auto publicKey = mCrypto.getPublicKey();

        std::cerr << "have pk\n";
        Payload ack{
            Payload::Type::__E2E_ACK__,
            std::string((const char*)publicKey.data(), publicKey.size())};
        std::cerr << "couldnt create payload\n";
        sessState = E2EState::ESTABLISHED;
        sessTimestamp = timestamp;
        return ack;
    } else {
        return {};
    }
}

std::optional<Payload> E2ESession::handleAck(Payload pl) {
    if (sessState != E2EState::INIT_SENT) {
        return {};
    }
    MemBuffer buf;
    buf << pl.data;
    std::vector<uint8_t> peerKey;
    buf >> peerKey;
    mCrypto.establish(peerKey);
    sessState = E2EState::ESTABLISHED;
    return {};
}

Payload E2ESession::encrypt(std::string_view str) {
    if (sessState == E2EState::ESTABLISHED) {
        std::vector<uint8_t> plaintext(str.begin(), str.end());
        auto eData = mCrypto.encrypt(plaintext);
        MemBuffer buf;
        buf << eData.nonce;
        buf << eData.ciphertext;
        buf << eData.tag;
        return Payload{Payload::Type::__E2E_MSG__,
                       std::string(buf.data(), buf.size())};
    } else {
        return Payload{Payload::Type::__PLAIN_TEXT__,
                       std::string(str.data(), str.size())};
    }
}

std::string E2ESession::decrypt(const Payload& pl) {
    if (sessState == E2EState::ESTABLISHED &&
        pl.type == Payload::Type::__E2E_MSG__) {
        MemBuffer buf;
        buf << pl.data;
        AESGCM::EncryptedData eData;
        buf >> eData.nonce;
        buf >> eData.ciphertext;
        buf >> eData.tag;

        auto msg = mCrypto.decrypt(eData);
        return std::string(msg.begin(), msg.end());
    } else if (pl.type == Payload::Type::__PLAIN_TEXT__) {
        return pl.data;
    } else {
        return "";
    }
}
