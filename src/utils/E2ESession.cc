#include "E2ESession.hh"

#include <openssl/bn.h>
#include <openssl/rand.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>

#include "utils/Cryptography/AESGCM.hh"
#include "utils/Cryptography/CryptoSession.hh"
#include "utils/MemBuffer.h"

std::optional<Payload> E2ESession::initiate() {
    std::cerr << "initiate:State is " << (int)sessState << "\n";
    std::vector<uint8_t> publicKey = mCrypto.getPublicKey();
    std::cout << "here1\n";
    uint64_t timestamp =
        std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "here2\n";
    E2EInit initpl = {.seqno = ++sequenceNo,
                      .timestamp = timestamp,
                      .key = std::move(publicKey)};
    std::cout << "here3\n";
    Payload pl;
    std::cout << "here4\n";
    pl << initpl;
    std::cout << "here5\n";
    sessState = E2EState::INIT_SENT;
    std::cout << "here6\n";
    sessTimestamp = timestamp;
    std::cerr << "initiate:State is " << (int)sessState << "\n";
    return pl;
}

std::optional<Payload> E2ESession::refresh() {
    std::cerr << "refresh:State is " << (int)sessState << "\n";
    oldCrypto = std::move(mCrypto);
    mCrypto = CryptoSession();
    std::cerr << "here123\n";
    std::vector<uint8_t> publicKey = mCrypto.getPublicKey();
    std::cerr << "here1\n";
    uint64_t timestamp =
        std::chrono::system_clock::now().time_since_epoch().count();
    std::cerr << "here11\n";
    E2EInit initpl = {.seqno = ++sequenceNo,
                      .timestamp = timestamp,
                      .key = std::move(publicKey)};
    std::cerr << "here2\n";
    Payload pl;
    pl << initpl;
    std::cerr << "here3\n";
    sessState = E2EState::REFRESH_SENT;
    sessTimestamp = timestamp;
    std::cerr << "refresh:State is " << (int)sessState << "\n";
    return pl;
}

std::optional<Payload> E2ESession::handleInit(Payload pl) {
    E2EInit initpl;
    pl >> initpl;
    if (initpl.seqno > sequenceNo ||
        (initpl.seqno == sequenceNo && initpl.timestamp < sessTimestamp)) {
        if (sessState == E2EState::ESTABLISHED) {
            std::cerr << "refreshing crypto session\n";
            oldCrypto = std::move(mCrypto);
            mCrypto = CryptoSession();
        }
        mCrypto.establish(initpl.key);
        auto publicKey = mCrypto.getPublicKey();
        E2EAck ack{initpl.seqno, std::move(publicKey)};
        std::cerr << "now seq no is " << initpl.seqno << "\n";
        Payload ackpl;
        ackpl << ack;
        sessState = E2EState::ESTABLISHED;
        std::cerr << "handleInit:State is " << (int)sessState << "\n";
        sessTimestamp = initpl.timestamp;
        sequenceNo = initpl.seqno;
        return ackpl;
    } else {
        return {};
    }
}

std::optional<Payload> E2ESession::handleAck(Payload pl) {
    std::cerr << "handleAck:State is " << (int)sessState << "\n";
    if (sessState != E2EState::INIT_SENT &&
        sessState != E2EState::REFRESH_SENT) {
        return {};
    }
    std::cerr << "handleAck:State is " << (int)sessState << "\n";
    E2EAck ackpl;
    pl >> ackpl;
    std::cerr << "seq: " << sequenceNo << "   rcvd seq: " << ackpl.seqno << "\n";
    if (ackpl.seqno != sequenceNo)
        return {};
    mCrypto.establish(ackpl.key);
    sessState = E2EState::ESTABLISHED;
    std::cerr << "handleAck:State is " << (int)sessState << "\n";

    return {};
}

Payload E2ESession::encrypt(std::string_view str) {
    std::cerr << "encrypt:State is " << (int)sessState << "\n";
    if (sessState == E2EState::ESTABLISHED) {
        std::cerr << "encrypting with key\n";
        std::vector<uint8_t> plaintext(str.begin(), str.end());
        auto eData = mCrypto.encrypt(plaintext);
        Payload pl;
        pl << E2EMsg{sequenceNo, eData};
        return pl;
    } else {
        std::cerr << "Encrypting plaintext\n";
        Payload pl;
        pl << E2EMsg{sequenceNo, std::string(str)};
        return pl;
    }
}

std::string E2ESession::decrypt(const Payload& pl) {
    std::cerr << "decrypt:State is " << (int)sessState << "\n";
    E2EMsg msg;
    pl >> msg;
    if (sessState == E2EState::ESTABLISHED &&
        pl.type == Payload::Type::__E2E_MSG__) {
        std::cerr << "hereeee\n";
        std::vector<uint8_t> plaintext;
        if (msg.seqno == sequenceNo)
            plaintext =
                mCrypto.decrypt(std::get<AESGCM::EncryptedData>(msg.data));
        else
            plaintext = oldCrypto.decrypt(std::get<AESGCM::EncryptedData>(msg.data));
        return std::string(plaintext.begin(), plaintext.end());
    } else if (pl.type == Payload::Type::__PLAIN_TEXT__) {
        return std::get<std::string>(msg.data);
    } else {
        return "";
    }
}
