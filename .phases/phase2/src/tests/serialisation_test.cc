// write a test for the Membuffer and MessagePacket classes using gtest framework
#include <gtest/gtest.h>
#include "MemBuffer.h"
#include "Packet.hh"

// should check that the MemBuffer class can write and read data correctly
TEST(MemBufferTest, WriteReadInt) {
    MemBuffer buffer(10);
    int value = 42;
    buffer << value;
    int readValue;
    buffer >> readValue;
    EXPECT_EQ(value, readValue);
}

// should check that the MemBuffer class can write and read strings correctly
TEST(MemBufferTest, WriteReadString) {
    MemBuffer buffer(50);
    std::string str = "Hello, World!";
    buffer << str;
    std::string readStr;
    buffer >> readStr;
    EXPECT_EQ(str, readStr);
}

// should check that the MessagePacket class can serialise and deserialise correctly
TEST(MessagePacketTest, SerialiseDeserialise) {
    std::string username = "user1";
    std::string message = "Hello!";
    MessagePacket packet(username, message);
    MemBuffer buffer(100);
    packet.serialise(buffer);   
    MessagePacket readPacket;
    readPacket.deserialise(buffer);
    EXPECT_EQ(packet.mPacketType, readPacket.mPacketType);
    // EXPECT_EQ(username, readPacket.getUsername());
    EXPECT_EQ(message, readPacket.getMessage());
}