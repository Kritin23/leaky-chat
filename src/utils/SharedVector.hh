#pragma once

#include <mutex>
#include <vector>

template <typename T>
class SharedVector {
  private:
    std::vector<T> mVector;
    std::mutex mMutex;

  public:
    void push_back(const T& item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mVector.push_back(item);
    }

    void push_back(T&& item) {
        std::lock_guard<std::mutex> lock(mMutex);
        mVector.push_back(std::move(item));
    }

    std::vector<T> getVector() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mVector;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mVector.size();
    }

    T& operator[](size_t index) {
        std::lock_guard<std::mutex> lock(mMutex);
        return mVector[index];
    }
};