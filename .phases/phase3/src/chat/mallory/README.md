# Phase 3 Mallory attacks

This proxy is for the Phase 3 certificate/proof-of-possession tests.

It intentionally does not use `<openssl/ssl.h>` and does not use OpenSSL DH APIs.

## Build

From `attacks/phase3/mallory`:

```bash
cmake -S . -B build
cmake --build build -j
```

## Generate Mallory's fake certificate/key

From `attacks/phase3/mallory`:

```bash
./generate_fake_cert.sh
```

The generated certificate is self-signed. It contains `127.0.0.1` so the identity check is not the reason it fails; the CA-chain verification should fail first.

## Attack 1: fake/untrusted certificate

From the repository root:

```bash
./attacks/phase3/mallory/build/mallory fake-cert
```

Run the Phase 3 client against Mallory:

```bash
./<phase3-client-binary> 127.0.0.1 10102
```

Expected result: the client rejects Mallory's certificate before sending the password, challenge, or DH public key.

## Attack 2: stolen legitimate certificate, no private key

Mallory uses the real `certs/server/server.crt`, but signs the client's challenge with `mallory.key`.

Start:

```bash
./attacks/phase3/mallory/build/mallory stolen-cert
```

Then run the Phase 3 client against:

```bash
./<phase3-client-binary> 127.0.0.1 10102
```

Expected result: certificate validation succeeds, the client sends its challenge, and proof-of-possession fails because Mallory cannot produce a signature matching the public key in the legitimate server certificate.
