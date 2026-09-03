#include <algorithm>
#include <barrier>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "chat/client.h"
#include "chat/server.h"
#include "utils/Packet.hh"

constexpr int NUM_CLIENTS = 10;
constexpr int PORT = 10101;

void runClient(int id,
               const std::vector<std::string>& receivers,
               const std::vector<std::string>& senders,
               std::barrier<>& loginBarrier) {
    Client client;

    std::string username = "user" + std::to_string(id);
    std::string message = "Hello from " + username;

    if (!client.setupConnection("127.0.0.1", PORT)) {
        std::cerr << "[Client " << id << "] failed to connect." << std::endl;
        return;
    }

    if (!client.login(username)) {
        std::cerr << "[Client " << id << "] failed to login." << std::endl;
        return;
    }

    std::cerr << "[Client " << id << "] logged in." << std::endl;

    // Make sure all 10 users are logged in before checking the user list.
    loginBarrier.arrive_and_wait();

    auto users = client.getUsers();

    if (users.size() != NUM_CLIENTS) {
        std::cerr << "[Client " << id << "] expected "
                  << NUM_CLIENTS << " users, got "
                  << users.size() << std::endl;
        return;
    }

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        std::string expectedUser = "user" + std::to_string(i);

        if (std::find(users.begin(), users.end(), expectedUser) == users.end()) {
            std::cerr << "[Client " << id << "] missing "
                      << expectedUser << std::endl;
            return;
        }
    }

    std::cerr << "[Client " << id << "] verified user list." << std::endl;

    // Send to our randomly selected receiver.
    client.connectTo(receivers[id]);
    client.send(message);

    std::cerr << "[Client " << id << "] sent message to "
              << receivers[id] << std::endl;

    // Wait for the message from the client that selected us.
    while (true) {
        auto pkt = client.poll();

        if (!pkt) {
            continue;
        }

        if (pkt->mPacketType != PacketType::MESSAGE) {
            continue;
        }

        auto msgPkt = getDerivedPacket<MessagePacket>(std::move(pkt));

        std::cerr << "[Client " << id << "] received from "
                  << msgPkt->getSender()
                  << ": "
                  << msgPkt->getMessage()
                  << std::endl;

        if (msgPkt->getSender() != senders[id]) {
            std::cerr << "[Client " << id << "] wrong sender. Expected "
                      << senders[id]
                      << ", got "
                      << msgPkt->getSender()
                      << std::endl;
            return;
        }

        std::string expectedMessage =
            "Hello from " + senders[id];

        if (msgPkt->getMessage() != expectedMessage) {
            std::cerr << "[Client " << id << "] wrong message. Expected "
                      << expectedMessage
                      << ", got "
                      << msgPkt->getMessage()
                      << std::endl;
            return;
        }

        std::cerr << "[Client " << id << "] message verified." << std::endl;
        break;
    }
}

int main() {
    std::cerr << "Starting multiple client test..." << std::endl;

    Server server;

    std::thread serverThread([&server]() {
        server.run();
    });

    std::vector<int> order(NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        order[i] = i;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::shuffle(order.begin(), order.end(), gen);

    std::vector<std::string> receivers(NUM_CLIENTS);
    std::vector<std::string> senders(NUM_CLIENTS);

    /*
     * Create a random cycle.
     *
     * For example:
     *
     * user3 -> user7
     * user7 -> user1
     * user1 -> user9
     * ...
     *
     * receivers[sender] = receiver
     * senders[receiver] = sender
     */
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        int sender = order[i];
        int receiver = order[(i + 1) % NUM_CLIENTS];

        receivers[sender] = "user" + std::to_string(receiver);
        senders[receiver] = "user" + std::to_string(sender);
    }

    std::barrier loginBarrier(NUM_CLIENTS);

    std::vector<std::thread> clients;
    clients.reserve(NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        clients.emplace_back(
            runClient,
            i,
            std::cref(receivers),
            std::cref(senders),
            std::ref(loginBarrier)
        );
    }

    for (auto& client : clients) {
        client.join();
    }

    // server.stop();
    serverThread.join();

    std::cerr << "Multiple client test completed." << std::endl;

    return 0;
}