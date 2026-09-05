# Phase 2 Mallory MITM proxy

This proxy is for the Phase 2 MITM experiment. It reuses the project's existing `CryptoSession`, `AESGCM`, and DH implementation rather than implementing a second crypto stack.

Topology:

    client -> Mallory:10101 -> server:10101

Mallory terminates two independent DH sessions:

    client <-> Mallory
    Mallory <-> server

Every encrypted application frame is decrypted at Mallory, logged as a printable ASCII view, and re-encrypted with the other session.

## Files

- `MalloryProxy.hh`
- `MalloryProxy.cpp`
- `main.cpp`
- `CMakeLists.txt.snippet`

## Integration

Put these three source files under:

    src/chat/mallory/

Add `CMakeLists.txt.snippet` to `src/chat/CMakeLists.txt`.

No normal server/client source changes are needed for the proxy itself.

## Client change for the attack

Your current `client_main.cpp` does not hard-code a host; the client eventually calls `Client::setupConnection(host, port)`. For the MITM run, make the caller supply Mallory's address instead of the server's address.

Use:

    host = 172.20.0.13
    port = 10101

Mallory then connects to:

    server = 172.20.0.10:10101

Do not change the real server address inside the server.

## Tampering

Normal:

    /workspace/build/bin/mallory

Flip one ciphertext bit on the client -> server leg:

    /workspace/build/bin/mallory --tamper c2s

Flip one ciphertext bit on the server -> client leg:

    /workspace/build/bin/mallory --tamper s2c

The receiver should reject the modified frame during AES-GCM authentication.

## Docker

Create:

    mkdir -p attacks/phase2/captures

Add a fourth compose service with IP `172.20.0.13` and mount `../../phase2/captures:/captures`.

Build with:

    docker exec leaky-mallory sh -c 'cd /workspace && cmake -S src -B build && cmake --build build -j'

Run with:

    docker exec -it leaky-mallory /workspace/build/bin/mallory

Capture both legs from Mallory:

    docker exec -it leaky-mallory tcpdump -i eth0 -nn -s 0 -w /captures/phase2-mitm.pcap tcp port 10101

## Expected log

After the client connects successfully, Mallory should show:

    [Mallory] Client <-> Mallory DH established
    [Mallory] Mallory <-> Server DH established

Then application traffic should look like:

    [Mallory] C -> S plaintext: ........THIS_IS_PHASE2........

The non-printable bytes are packet framing/fields; the chat text should be readable in the middle of the log.
