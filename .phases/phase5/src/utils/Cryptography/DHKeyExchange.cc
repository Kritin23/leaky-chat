#include "DHKeyExchange.hh"

#include <openssl/rand.h>

#include <stdexcept>
#include <iostream>
namespace {

constexpr const char* GROUP14_PRIME =
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"
    "8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"
    "302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"
    "A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE"
    "649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA"
    "8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966"
    "D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772"
    "C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
    "3995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFF"
    "FFFFFFFF";

constexpr unsigned int PRIVATE_KEY_BYTES = 32;

}

DHKeyExchange::DHKeyExchange()
    : mPrivateKey(BN_new()),
      mPublicKey(BN_new())
{

    if (!mPrivateKey || !mPublicKey)
        throw std::runtime_error("BN_new failed");

    unsigned char randomBytes[PRIVATE_KEY_BYTES];

    if (RAND_bytes(randomBytes, sizeof(randomBytes)) != 1)
        throw std::runtime_error("RAND_bytes failed");

    if (!BN_bin2bn(
            randomBytes,
            sizeof(randomBytes),
            mPrivateKey)) {
        throw std::runtime_error("BN_bin2bn failed");
    }

    BIGNUM* publicKey =
        modExp(generator(), mPrivateKey, prime());

    if (!BN_copy(mPublicKey, publicKey)) {
        BN_free(publicKey);
        throw std::runtime_error("BN_copy failed");
    }
    
    BN_free(publicKey);
}

DHKeyExchange::DHKeyExchange(DHKeyExchange&& other) noexcept
    : mPrivateKey(other.mPrivateKey),
      mPublicKey(other.mPublicKey)
{
    other.mPrivateKey = nullptr;
    other.mPublicKey = nullptr;
}

DHKeyExchange& DHKeyExchange::operator=(DHKeyExchange&& other) noexcept
{
    if (this != &other) {
        BN_free(mPublicKey);
        BN_clear_free(mPrivateKey);

        mPrivateKey = other.mPrivateKey;
        mPublicKey = other.mPublicKey;

        other.mPrivateKey = nullptr;
        other.mPublicKey = nullptr;
    }

    return *this;
}

DHKeyExchange::~DHKeyExchange()
{
    BN_free(mPublicKey);
    BN_clear_free(mPrivateKey);
}

const BIGNUM* DHKeyExchange::prime()
{
    static BIGNUM* p = [] {
        BIGNUM* value = BN_new();

        if (!value)
            throw std::runtime_error("BN_new failed");

        if (!BN_hex2bn(&value, GROUP14_PRIME)) {
            BN_free(value);
            throw std::runtime_error("BN_hex2bn failed");
        }

        return value;
    }();

    return p;
}

const BIGNUM* DHKeyExchange::generator()
{
    static BIGNUM* g = [] {
        BIGNUM* value = BN_new();

        if (!value)
            throw std::runtime_error("BN_new failed");

        if (!BN_set_word(value, 2)) {
            BN_free(value);
            throw std::runtime_error("BN_set_word failed");
        }

        return value;
    }();

    return g;
}

BIGNUM* DHKeyExchange::modExp(
    const BIGNUM* base,
    const BIGNUM* exponent,
    const BIGNUM* modulus)
{
    BIGNUM* result = BN_new();
    BIGNUM* b = BN_new();
    BIGNUM* e = BN_dup(exponent);
    BN_CTX* ctx = BN_CTX_new();

    if (!result || !b || !e || !ctx)
        throw std::runtime_error("BIGNUM allocation failed");

    BN_one(result);
    BN_mod(b, base, modulus, ctx);

    while (!BN_is_zero(e)) {
        if (BN_is_odd(e))
            BN_mod_mul(result, result, b, modulus, ctx);

        BN_mod_mul(b, b, b, modulus, ctx);
        BN_rshift1(e, e);
    }

    BN_free(b);
    BN_free(e);
    BN_CTX_free(ctx);

    return result;
}

std::vector<std::uint8_t>
DHKeyExchange::getPublicKey() const
{
    std::vector<std::uint8_t> result(256);

    if (BN_bn2binpad(
            mPublicKey,
            result.data(),
            result.size()) != static_cast<int>(result.size())) {
        throw std::runtime_error("BN_bn2binpad failed");
    }

    return result;
}

std::vector<std::uint8_t>
DHKeyExchange::computeSharedSecret(
    const std::vector<std::uint8_t>& peerPublicKey) const
{
    BIGNUM* peer =
        BN_bin2bn(
            peerPublicKey.data(),
            peerPublicKey.size(),
            nullptr);

    if (!peer)
        throw std::runtime_error("BN_bin2bn failed");

    // shared = peerPublic^private mod p
    BIGNUM* shared =
        modExp(peer, mPrivateKey, prime());

    BN_free(peer);

    std::vector<std::uint8_t> result(256);

    if (BN_bn2binpad(
            shared,
            result.data(),
            result.size()) != static_cast<int>(result.size())) {
        BN_clear_free(shared);
        throw std::runtime_error("BN_bn2binpad failed");
    }

    BN_clear_free(shared);

    return result;
}