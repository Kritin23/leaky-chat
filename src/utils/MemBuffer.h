#pragma once

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

class MemBuffer {
  private:
    char* data_;
    size_t begin;
    size_t end;
    size_t capacity_;

  public:
    MemBuffer() : data_(nullptr), begin(0), end(0), capacity_(0) {}

    /// Constructor that allocates @p sz bytes for buffer
    MemBuffer(size_t sz)
        : data_(new char[sz]), begin(0), end(0), capacity_(sz) {}

    size_t capacity() const { return capacity_; }

    size_t size() const { return end - begin; }

    void resize(size_t sz);

    // Moves data to the left, freeing up space after end
    void relocate();

    void ensureFreeSpace(size_t sz);

    void write_bytes(const void* src, size_t len);

    template <std::integral T>
        requires(!std::is_same_v<T, bool>)
    MemBuffer& operator<<(T val) {
        write_bytes(&val, sizeof(T));
        return *this;
    }

    template <std::integral T>
        requires(!std::is_same_v<T, bool>)
    MemBuffer& operator<<(std::vector<T> vec) {
        *this << vec.size();
        for (const auto& val : vec) {
            *this << val;
        }
        return *this;
    }

    MemBuffer& operator<<(std::string_view sv);
    MemBuffer& operator<<(std::vector<std::string> vec);

    void consume(size_t len);

    const char* data() const { return data_ + begin; }
    char* data() { return data_ + begin; }
    std::string_view view() const { return {data(), size()}; }

    template <std::integral T>
        requires(!std::is_same_v<T, bool>)
    MemBuffer& operator>>(T& val) {
        read(&val, sizeof(T));
        return *this;
    }
    template <std::integral T>
        requires(!std::is_same_v<T, bool>)
    MemBuffer& operator>>(std::vector<T>& vec) {
        size_t vecSize;
        *this >> vecSize;
        vec.resize(vecSize);
        for (auto& val : vec) {
            *this >> val;
        }
        return *this;
    }

    MemBuffer& operator>>(std::string& str);
    MemBuffer& operator>>(std::vector<std::string>& vec);

    size_t read(void* dest, size_t len);
};