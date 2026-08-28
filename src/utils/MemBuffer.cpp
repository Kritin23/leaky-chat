
#include "MemBuffer.h"

#include <string>
#include <cstring>
#include <stdexcept>

void MemBuffer::resize(size_t sz) {
    if (size() < sz) {
        char* newBuffer = new char[sz];
        std::memcpy(newBuffer, data_ + begin, end - begin);
        end = end - begin;
        begin = 0;
        data_ = newBuffer;
    }
}

void MemBuffer::relocate() {
    if (begin > 0) {
        memmove(data_, data_ + begin, end - begin);
        end = end - begin;
        begin = 0;
    }
}

void MemBuffer::ensureFreeSpace(size_t sz) {
    if (capacity() - size() < sz) {
        resize(2 * capacity());
    } else if (capacity() - end < sz) {
        relocate();
    }
}

void MemBuffer::write_bytes(const void* src, size_t len) {
    ensureFreeSpace(len);
    std::memcpy(data_ + end, src, len);
    end += len;
}

MemBuffer& MemBuffer::operator<<(std::string_view sv) {
    *this << sv.size();
    write_bytes(sv.data(), sv.size());
    return *this;
}

MemBuffer& MemBuffer::operator<<(std::vector<std::string> vec) {
    *this << vec.size();
    for (const auto& str : vec) {
        *this << str;
    }
    return *this;
}

void MemBuffer::consume(size_t len) {
    if (len > size()) {
        throw std::out_of_range("MemBuffer::consume underflow");
    }
    begin += len;
    if (begin == end) {
        begin = 0;
        end = 0;
    }
}

size_t MemBuffer::read(void* dest, size_t len) {
    size_t bytes_to_read = (std::min)(len, size());
    if (bytes_to_read > 0) {
        std::memcpy(dest, data_ + begin, bytes_to_read);
        // begin += bytes_to_read; // -> I guess this is redundant and logically
        // incorrect?
        consume(bytes_to_read);
    }
    return bytes_to_read;
}

MemBuffer& MemBuffer::operator>>(std::string& str) {
    size_t sz;
    *this >> sz;
    str.resize(sz);
    if (sz > 0) {
        read(str.data(), sz);
    }
    return *this;
}

MemBuffer& MemBuffer::operator>>(std::vector<std::string>& vec) {
    size_t vecSize;
    *this >> vecSize;
    vec.resize(vecSize);
    for (auto& str : vec) {
        *this >> str;
    }
    return *this;
}