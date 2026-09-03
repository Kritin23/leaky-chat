#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

struct Payload {
    enum class Type : std::uint8_t {
        __PLAIN_TEXT__,
        __E2E_INIT__,
        __E2E_ACK__,
        __E2E_MSG__
    };
    Type type;
    std::string data;

    bool isE2EControl() const {
        return type == Type::__E2E_INIT__ || type == Type::__E2E_ACK__;
    }
};

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

    MemBuffer(const MemBuffer&) = delete;
    MemBuffer& operator=(const MemBuffer&) = delete;

    MemBuffer(MemBuffer&& other) {
        data_ = other.data_;
        other.data_ = nullptr;

        begin = other.begin;
        end = other.end;
        capacity_ = other.capacity_;
        other.begin = other.end = other.capacity_ = 0;
    }
    MemBuffer& operator=(MemBuffer&& other) {
        if (this != &other) {
            free();
            data_ = other.data_;
            other.data_ = nullptr;

            begin = other.begin;
            end = other.end;
            capacity_ = other.capacity_;
            other.begin = other.end = other.capacity_ = 0;
        }
        return *this;
    }

    void free() {
        if (data_) {
            delete[] data_;
            data_ = nullptr;
            begin = end = capacity_ = 0;
        }
    }

    ~MemBuffer() { free(); }

    size_t capacity() const { return capacity_; }

    size_t size() const { return end - begin; }

    size_t availableSpace() const { return capacity_ - end; }

    size_t freeSpace() const { return capacity() - size();}

    void resize(size_t sz);

    // Moves data to the left, freeing up space after end
    void relocate();

    void ensureFreeSpace(size_t sz);

    void write_bytes(const void* src, size_t len);

    template <std::integral T>
        requires (!std::is_same_v<T, bool>)
    MemBuffer& operator<<(T val) {
        write_bytes(&val, sizeof(T));
        return *this;
    }

    template <std::integral T>
        requires (!std::is_same_v<T, bool>)
    MemBuffer& operator<<(std::vector<T> vec) {
        *this << vec.size();
        for (const auto& val : vec) {
            *this << val;
        }
        return *this;
    }
    template <typename T, std::size_t N>
    MemBuffer& operator<<(const std::array<T, N>& data) {
        write_bytes(data.data(), N * sizeof(T));
        return *this;
    }

    MemBuffer& operator<<(std::string_view sv);
    MemBuffer& operator<<(std::vector<std::string> vec);
    MemBuffer& operator<<(Payload payload);

    void consume(size_t len);

    const char* data() const { return data_ + begin; }
    char* data() { return data_ + begin; }
    std::string_view view() const { return {data(), size()}; }

    template <std::integral T>
        requires (!std::is_same_v<T, bool>)
    MemBuffer& operator>>(T& val) {
        read(&val, sizeof(T));
        return *this;
    }
    template <std::integral T>
        requires (!std::is_same_v<T, bool>)
    MemBuffer& operator>>(std::vector<T>& vec) {
        size_t vecSize;
        *this >> vecSize;
        vec.resize(vecSize);
        for (auto& val : vec) {
            *this >> val;
        }
        return *this;
    }

    template <typename T, std::size_t N>
    MemBuffer& operator>>(std::array<T, N>& data) {
        read(data.data(), N * sizeof(T));
        return *this;
    }

    MemBuffer& operator>>(std::string& str);
    MemBuffer& operator>>(std::vector<std::string>& vec);
    MemBuffer& operator>>(Payload& payload);

    size_t read(void* dest, size_t len);
};