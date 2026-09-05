#include "MalloryProxy.hh"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

#include "utils/Packet.hh"

static int makeBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;

    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

MalloryProxy::MalloryProxy(const std::string& serverHost,
                           uint16_t serverPort,
                           uint16_t listenPort,
                           bool tamperClientToServer)
    : mServerHost(serverHost),
      mServerPort(serverPort),
      mListenPort(listenPort),
      mTamperClientToServer(tamperClientToServer),
      mClientSide(),
      mServerSide(serverHost, serverPort) {}

MalloryProxy::~MalloryProxy() {
    if (mListenSocket != -1) {
        ::close(mListenSocket);
        mListenSocket = -1;
    }

    mClientSide.close();
    mServerSide.close();
}

int MalloryProxy::createListenSocket() {
    mListenSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (mListenSocket < 0) {
        std::cerr << "Mallory: socket() failed: " << std::strerror(errno)
                  << '\n';
        return -1;
    }

    int reuse = 1;

    if (setsockopt(
            mListenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        std::cerr << "Mallory: setsockopt() failed: " << std::strerror(errno)
                  << '\n';
        ::close(mListenSocket);
        mListenSocket = -1;
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(mListenPort);

    if (::bind(mListenSocket,
               reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) < 0) {
        std::cerr << "Mallory: bind() failed: " << std::strerror(errno) << '\n';
        ::close(mListenSocket);
        mListenSocket = -1;
        return -1;
    }

    if (::listen(mListenSocket, SOMAXCONN) < 0) {
        std::cerr << "Mallory: listen() failed: " << std::strerror(errno)
                  << '\n';
        ::close(mListenSocket);
        mListenSocket = -1;
        return -1;
    }

    return 0;
}

void MalloryProxy::inspectPacket(const char* direction, const Packet& packet) {
    if (packet.mPacketType != PacketType::MESSAGE)
        return;

    const auto* messagePacket = dynamic_cast<const MessagePacket*>(&packet);

    if (!messagePacket)
        return;

    std::cerr << "\033[31m";
    std::cerr << "[MALLORY HEARD] ";
    std::cerr << direction;
    std::cerr << " receiver=" << messagePacket->getReceiver();
    std::cerr << " message=" << messagePacket->getMessage();
    std::cerr << "\033[0m\n";
}

void MalloryProxy::relayClientToServer() {
    while (mClientSide.connected() && mServerSide.connected()) {
        auto packet = mClientSide.receivePacket();

        if (!packet) {
            if (!mClientSide.connected())
                break;

            continue;
        }

        inspectPacket("CLIENT -> SERVER", *packet);

        if (mTamperClientToServer) {
            std::cerr << "Mallory: tampered CLIENT -> SERVER ciphertext\n";
        }

        if (mServerSide.sendPacket(*packet, mTamperClientToServer) != 0) {
            std::cerr << "Mallory: failed to forward CLIENT -> SERVER\n";
            break;
        }
    }

    mClientSide.close();
    mServerSide.close();
}

void MalloryProxy::relayServerToClient() {
    while (mServerSide.connected() && mClientSide.connected()) {
        auto packet = mServerSide.receivePacket();

        if (!packet) {
            if (!mServerSide.connected())
                break;

            continue;
        }

        inspectPacket("SERVER -> CLIENT", *packet);

        if (mClientSide.sendPacket(*packet) != 0) {
            std::cerr << "Mallory: failed to forward SERVER -> CLIENT\n";
            break;
        }
    }

    mServerSide.close();
    mClientSide.close();
}

int MalloryProxy::run() {
    if (createListenSocket() != 0)
        return -1;

    std::cerr << "Mallory listening on 0.0.0.0:" << mListenPort << '\n';

    std::cerr << "Mallory forwarding to " << mServerHost << ':' << mServerPort
              << '\n';

    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientSocket = accept(mListenSocket,
                              reinterpret_cast<sockaddr*>(&clientAddress),
                              &clientAddressLength);

    if (clientSocket < 0) {
        std::cerr << "Mallory: accept() failed: " << std::strerror(errno)
                  << '\n';
        return -1;
    }

    char clientIP[INET_ADDRSTRLEN]{};

    inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP));

    std::cerr << "Mallory accepted client " << clientIP << '\n';

    if (mClientSide.setSocket(clientSocket) != 0) {
        std::cerr << "Mallory: failed to attach client socket\n";
        ::close(clientSocket);
        return -1;
    }

    if (mClientSide.performServerHandshake() != 0) {
        std::cerr << "Mallory: client-side handshake failed\n";
        mClientSide.close();
        return -1;
    }

    std::cerr << "Mallory: client-side crypto session established\n";

    if (mServerSide.connect() != 0) {
        std::cerr << "Mallory: server-side connection failed\n";
        mClientSide.close();
        return -1;
    }

    std::cerr << "Mallory: server-side crypto session established\n";

    if (makeBlocking(mClientSide.getFd()) != 0 ||
        makeBlocking(mServerSide.getFd()) != 0) {
        std::cerr << "Mallory: failed to make relay sockets blocking\n";
        mClientSide.close();
        mServerSide.close();
        return -1;
    }

    std::thread clientToServer(&MalloryProxy::relayClientToServer, this);

    std::thread serverToClient(&MalloryProxy::relayServerToClient, this);

    clientToServer.join();
    serverToClient.join();

    return 0;
}