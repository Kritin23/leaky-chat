#pragma once

#include <poll.h>

#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>

#include "NetworkHandler.hh"

/// Socket ID
using SID = size_t;

class SocketSet {
  private:
    std::vector<NetworkHandler> data;
    std::vector<pollfd> readPollfd;

  public:
    SID insert(NetworkHandler&& nh);
    NetworkHandler* waitForRead();

    NetworkHandler& operator[](SID idx) { return data[idx]; }
    const NetworkHandler& operator[](SID idx) const { return data[idx]; }
};

template <typename T>
class SocketMap {
  protected:
    std::deque<T> data;

    void ensure(size_t size) {
        if (data.size() < size) {
            data.resize(size);
        }
    }

  public:
    void insert(SID id, T&& value) {
        ensure(id + 1);
        data[id] = std::move(value);
    }
    void insert(SID id, const T& value) {
        ensure(id + 1);
        data[id] = value;
    }

    template <typename... Args>
    void emplace(SID id, Args&&... args) {
        ensure(id + 1);
        data[id] = T(std::forward<Args>(args)...);
    }

    T& operator[](SID idx) { return data[idx]; }
    const T& operator[](SID idx) const { return data[idx]; }

    size_t size() const { return data.size(); }
};

template <typename T>
struct DerefHash {
    using is_transparent = void;

    std::size_t operator()(const T* ptr) const { return std::hash<T>{}(*ptr); }
    std::size_t operator()(const T& val) const { return std::hash<T>{}(val); }
};

template <typename T>
struct DerefEqual {
    using is_transparent = void;

    bool operator()(const T* a, const T* b) const { return *a == *b; }
    bool operator()(const T* a, const T& b) const { return *a == b; }
    bool operator()(const T& a, const T* b) const { return a == *b; }
};

template <typename T>
class IndexedSocketMap : public SocketMap<T> {
    std::unordered_map<const T*, SID, DerefHash<T>, DerefEqual<T>> reverseMap;

  public:
    template <typename U>
    void insert(SID id, U&& value) {
        if (id < this->size()) {
            const T& old_val = (*this)[id];
            reverseMap.erase(old_val);
        }
        SocketMap<T>::insert(id, std::forward<U>(value));
        const T* element_ptr = &((*this)[id]);
        reverseMap[element_ptr] = id;
    }

    template <typename... Args>
    void emplace(SID id, Args&&... args) {
        if (id < this->size()) {
            const T& old_val = (*this)[id];
            reverseMap.erase(old_val);
        }
        SocketMap<T>::emplace(id, std::forward<Args>(args)...);
        const T* element_ptr = &((*this)[id]);
        reverseMap[element_ptr] = id;
    }

    std::optional<SID> getSID(const T& val) const {
        auto it = reverseMap.find(val);
        if (it != reverseMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};