#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include "utils/Packet.hh"
#include "chat/server.h"
#include "chat/client.h"

Server* server;

void runServer() {
    server->run();
}

void runClient1() {
    client_impl::Client client;

    if (!client.setupConnection("127.0.0.1", 10101)) {
        std::cerr << "Client 1 failed to connect to server." << std::endl;
        return;
    }

    if (!client.login("testuser1")) {
        std::cerr << "Client 1 failed to login." << std::endl;
        return;
    }

    if (!client.connectTo("testuser2")) {
        std::cerr << "Client 1 failed to connect to testuser2." << std::endl;
        return;
    }

    client.send("Hello from testuser1!");

    while (true) {
        auto pkt = client.poll();

        if (!pkt) {
            continue;
        }

        if (pkt->mPacketType != PacketType::MESSAGE) {
            continue;
        }

        auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));

        std::cerr << "Client 1 received from "
                  << msgPkt->getSender()
                  << ": "
                  << msgPkt->getMessage()
                  << std::endl;

        if (msgPkt->getSender() != "testuser2") {
            std::cerr << "Client 1 received wrong sender." << std::endl;
            return;
        }

        if (msgPkt->getMessage() != "Hello from testuser2!") {
            std::cerr << "Client 1 received wrong message." << std::endl;
            return;
        }

        std::cerr << "Client 1 received correct message." << std::endl;
        break;
    }
}

void runClient2() {
    client_impl::Client client;

    if (!client.setupConnection("127.0.0.1", 10101)) {
        std::cerr << "Client 2 failed to connect to server." << std::endl;
        return;
    }

    if (!client.login("testuser2")) {
        std::cerr << "Client 2 failed to login." << std::endl;
        return;
    }

    if (!client.connectTo("testuser1")) {
        std::cerr << "Client 2 failed to connect to testuser1." << std::endl;
        return;
    }

    client.send("Hello from testuser2!");

    while (true) {
        auto pkt = client.poll();

        if (!pkt) {
            continue;
        }

        if (pkt->mPacketType != PacketType::MESSAGE) {
            continue;
        }

        auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));

        std::cerr << "Client 2 received from "
                  << msgPkt->getSender()
                  << ": "
                  << msgPkt->getMessage()
                  << std::endl;

        if (msgPkt->getSender() != "testuser1") {
            std::cerr << "Client 2 received wrong sender." << std::endl;
            return;
        }

        if (msgPkt->getMessage() != "Hello from testuser1!") {
            std::cerr << "Client 2 received wrong message." << std::endl;
            return;
        }

        std::cerr << "Client 2 received correct message." << std::endl;
        break;
    }
}

int main() {
    std::cerr << "Starting server and clients..." << std::endl;

    Server serverInstance;
    server = &serverInstance;

    std::thread serverThread(runServer);

    std::thread client1Thread(runClient1);
    std::thread client2Thread(runClient2);

    client1Thread.join();
    client2Thread.join();

    // server->stop();
    serverThread.join();

    std::cerr << "System test completed." << std::endl;

    return 0;
}