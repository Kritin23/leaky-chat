// int his test, we shall spawn a server and a client, and have the client send a message to the server. The server should receive the message and print it to stdout.
// we wont be using gtest for this test, it will be a simple test that runs the server and client in separate threads, and checks if the message is received by the server.
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "utils/Packet.hh"
#include "chat/server.h"
#include "chat/client.h"

void runServer() {
    Server server;
    server.run();
}

void runClient1() {
    Client client;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for server to start
    if(!client.setupConnection("127.0.0.1", 10101)) {
        std::cerr << "Client 1 failed to connect to server." << std::endl;
        return;
    }
    client.login("testuser1");
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for server to process login
    client.connectTo("testuser2");
    client.send("Hello, World!");
}

void runClient2() {
    Client client;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for server to start

    if(!client.setupConnection("127.0.0.1", 10101)) {
        std::cerr << "Client 2 failed to connect to server." << std::endl;
        return;
    }
    client.login("testuser2");
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for server to process login
    client.connectTo("testuser1");
    auto users = client.getUsers();
    std::cerr << "User list: ";
    for (const auto& user : users) {
        std::cerr << user << " ";
    }
    std::cerr << std::endl;
    client.send("Hello, testuser1!");
}

int main(){
    std::cerr << "Starting server and clients..." << std::endl;
    // return 0;
    std::thread serverThread(runServer);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for server to start
    std::thread client1Thread(runClient1);
    std::thread client2Thread(runClient2);

    client1Thread.join();
    client2Thread.join();
    serverThread.join();

    return 0;
}