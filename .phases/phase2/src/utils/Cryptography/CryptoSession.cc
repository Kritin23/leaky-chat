#include "CryptoSession.hh"

#include "KeyDerivation.hh"

#include <openssl/crypto.h>

#include <stdexcept>

std::vector<std::uint8_t>
CryptoSession::getPublicKey() const
{
    return mDH.getPublicKey();
}

void CryptoSession::establish(
    const std::vector<std::uint8_t>& peerPublicKey)
{
    if (mEstablished)
        throw std::runtime_error(
            "CryptoSession is already established");

    const auto sharedSecret =
        mDH.computeSharedSecret(peerPublicKey);

    const auto key = KeyDerivation::deriveKey(sharedSecret);

    std::copy(key.begin(), key.end(), mKey.begin());

    mEstablished = true;
}

bool CryptoSession::isEstablished() const
{
    return mEstablished;
}

AESGCM::EncryptedData
CryptoSession::encrypt(
    const std::vector<std::uint8_t>& plaintext) const
{
    if (!mEstablished)
        throw std::runtime_error(
            "CryptoSession is not established");

    return AESGCM::encrypt(mKey, plaintext);
}

std::vector<std::uint8_t>
CryptoSession::decrypt(
    const AESGCM::EncryptedData& encrypted) const
{
    if (!mEstablished)
        throw std::runtime_error(
            "CryptoSession is not established");

    return AESGCM::decrypt(mKey, encrypted);
}
