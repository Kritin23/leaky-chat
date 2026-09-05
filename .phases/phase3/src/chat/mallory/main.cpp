#include "MalloryProxy.hh"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <fake-cert|stolen-cert>\n";

        return 1;
    }

    MalloryProxy proxy(
        10101,
        "172.20.0.10",
        10101,
        "/workspace/src/chat/mallory/certs/mallory.crt",
        "/workspace/certs/server/server.crt",
        "/workspace/src/chat/mallory/certs/mallory.key");

    if (std::string(argv[1]) == "fake-cert") {
        return proxy.runFakeCertificateAttack();
    }

    if (std::string(argv[1]) == "stolen-cert") {
        return proxy.runStolenCertificateAttack();
    }

    std::cerr
        << "Unknown mode: "
        << argv[1]
        << '\n';

    return 1;
}