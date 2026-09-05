#include "E2ESession.hh"

#include <openssl/bn.h>
#include <openssl/rand.h>

#include <iostream>
#include <chrono>
#include <optional>
#include <string_view>
#include <iostream>
#include <utility>

#include "utils/Cryptography/AESGCM.hh"
#include "utils/Cryptography/CryptoSession.hh"
#include "utils/MemBuffer.h"

std::optional<Payload> E2ESession::initiate() {
    std::vector<uint8_t> publicKey = mCrypto.getPublicKey();
    uint64_t timestamp =
        std::chrono::system_clock::now().time_since_epoch().count();
    E2EInit initpl = {.seqno = ++sequenceNo,
                      .timestamp = timestamp,
                      .key = std::move(publicKey)};
    Payload pl;
    pl << initpl;
    sessState = E2EState::INIT_SENT;
    sessTimestamp = timestamp;
    return pl;
}

std::optional<Payload> E2ESession::refresh() {
    oldCrypto = CryptoSession();
    std::swap(oldCrypto, mCrypto);
    std::vector<uint8_t> publicKey = mCrypto.getPublicKey();
    uint64_t timestamp =
        std::chrono::system_clock::now().time_since_epoch().count();
    E2EInit initpl = {.seqno = ++sequenceNo,
                      .timestamp = timestamp,
                      .key = std::move(publicKey)};
    Payload pl;
    pl << initpl;
    sessState = E2EState::REFRESH_SENT;
    sessTimestamp = timestamp;
    return pl;
}

std::optional<Payload> E2ESession::handleInit(Payload pl) {
    E2EInit initpl;
    pl >> initpl;
    if (initpl.seqno > sequenceNo ||
        (initpl.seqno == sequenceNo && initpl.timestamp < sessTimestamp)) {
        if (sessState == E2EState::ESTABLISHED) {
            oldCrypto = CryptoSession();
            std::swap(oldCrypto, mCrypto);
        }
        mCrypto.establish(initpl.key);

        std::cerr << "est\n";

        auto publicKey = mCrypto.getPublicKey();

        std::cerr << "have pk\n";
        Payload ack{
            Payload::Type::__E2E_ACK__,
            std::string((const char*)publicKey.data(), publicKey.size())};
        std::cerr << "couldnt create payload\n";
        sessState = E2EState::ESTABLISHED;
        sessTimestamp = initpl.timestamp;
        sequenceNo = initpl.seqno;
        return ack;
    } else {
        return {};
    }
}

std::optional<Payload> E2ESession::handleAck(Payload pl) {
    if (sessState != E2EState::INIT_SENT && sessState!=E2EState::REFRESH_SENT) {
        return {};
    }
    E2EAck ackpl;
    pl >> ackpl;
    if (ackpl.seqno != sequenceNo)
        return {};
    mCrypto.establish(ackpl.key);
    sessState = E2EState::ESTABLISHED;
    return {};
}

Payload E2ESession::encrypt(std::string_view str) {
    if (sessState == E2EState::ESTABLISHED) {
        std::vector<uint8_t> plaintext(str.begin(), str.end());
        auto eData = mCrypto.encrypt(plaintext);
        MemBuffer buf(64);
        buf << sequenceNo;
        buf << eData.nonce;
        buf << eData.ciphertext;
        buf << eData.tag;
        return Payload{Payload::Type::__E2E_MSG__,
                       std::string(buf.data(), buf.size())};
    } else {
        std::cerr << "Encrypting plaintext\n";
        return Payload{Payload::Type::__PLAIN_TEXT__,
                       std::string(str.data(), str.size())};
    }
}

std::string E2ESession::decrypt(const Payload& pl) {
    if (sessState == E2EState::ESTABLISHED &&
        pl.type == Payload::Type::__E2E_MSG__) {
        MemBuffer buf(64);
        buf << pl.data;
        uint32_t seqNo;
        buf >> seqNo;
        AESGCM::EncryptedData eData;
        buf >> eData.nonce;
        buf >> eData.ciphertext;
        buf >> eData.tag;
        std::vector<uint8_t> msg;
        if (seqNo == sequenceNo)
            msg = mCrypto.decrypt(eData);
        else
            msg = oldCrypto.decrypt(eData);
        return std::string(msg.begin(), msg.end());
    } else if (pl.type == Payload::Type::__PLAIN_TEXT__) {
        return pl.data;
    } else {
        return "";
    }
}
