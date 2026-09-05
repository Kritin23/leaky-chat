#include "SocketSet.hh"

#include <poll.h>

#include "utils/NetworkHandler.hh"

SID SocketSet::insert(NetworkHandler&& nh) {
    size_t id = data.size();
    readPollfd.push_back({nh.getFd(), POLLIN, 0});
    data.emplace_back(std::move(nh));
    return id;
}

NetworkHandler* SocketSet::waitForRead() {
    static size_t idx = 0;
    // std::cout << "Waiting for read on " << readPollfd.size() << " sockets" <<
    // std::endl;
    poll(readPollfd.data(), readPollfd.size(), 1);
    // std::cout << "Poll returned, checking for ready sockets" << std::endl;
    for (size_t count = 0; count < data.size();
         count++, idx = (idx + 1) % data.size()) {
        if (readPollfd[idx].revents > 0) {
            return &data[idx];
        }
    }
    return nullptr;
}