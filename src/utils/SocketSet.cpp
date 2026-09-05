#include "SocketSet.hh"

#include <poll.h>
#include <sys/poll.h>

#include <iostream>

#include "utils/NetworkHandler.hh"

SID SocketSet::insert(NetworkHandler&& nh) {
    size_t id = data.size();
    data.emplace_back(std::move(nh));
    numActiveConnections++;
    return id;
}

SID SocketSet::waitForRead() {
    static size_t idx = 0;
    // std::cout << "Waiting for read on " << readPollfd.size() << " sockets" <<
    // std::endl;
    std::vector<pollfd> activeConnections;
    for(auto& nh : data) {
        if (nh.connected()) {
            activeConnections.push_back({nh.getFd(), POLLIN, 0});
        }
    }
    numActiveConnections = activeConnections.size();
    if (activeConnections.size() == 0)
        return -1;
    poll(activeConnections.data(), activeConnections.size(), 1000);

    // std::cout << "Poll returned, checking for ready sockets" << std::endl;
    for (size_t count = 0; count < data.size();
         count++, idx = (idx + 1) % data.size()) {
        if (activeConnections[idx].revents > 0) {
            return idx;
        }
    }
    return -1;
}