#include "MalloryProxy.hh"

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string serverHost = "172.20.0.10";
    uint16_t serverPort = 10101;
    uint16_t listenPort = 10101;
    bool tamperClientToServer = false;

    if (argc > 1)
        serverHost = argv[1];

    if (argc > 2)
        serverPort = static_cast<uint16_t>(std::stoi(argv[2]));

    if (argc > 3)
        listenPort = static_cast<uint16_t>(std::stoi(argv[3]));

    if (argc > 4 && std::string(argv[4]) == "--tamper")
        tamperClientToServer = true;

    MalloryProxy mallory(
        serverHost,
        serverPort,
        listenPort,
        tamperClientToServer);

    return mallory.run();
}