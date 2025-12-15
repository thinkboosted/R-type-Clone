#include <gtest/gtest.h>
#include "../NetworkManager.hpp"
#include <thread>
#include <chrono>

class NetworkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(NetworkManagerTest, Instantiation) {
    EXPECT_NO_THROW({
        rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556");
    });
}

TEST_F(NetworkManagerTest, BindDoesNotCrash) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5557", "tcp://127.0.0.1:5558");
    nm.init();
    EXPECT_NO_THROW(nm.bind(4242));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, ConnectDoesNotCrash) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5559", "tcp://127.0.0.1:5560");
    nm.init();
    EXPECT_NO_THROW(nm.connect("127.0.0.1", 4242));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, EndToEndCommunication) {
    // This test simulates a server and a client exchanging messages
    rtypeEngine::NetworkManager server("tcp://127.0.0.1:5571", "tcp://127.0.0.1:5572");
    rtypeEngine::NetworkManager client("tcp://127.0.0.1:5573", "tcp://127.0.0.1:5574");

    server.init();
    client.init();

    // Server binds to port 4243
    server.bind(4243);
    
    // Client connects to server
    client.connect("127.0.0.1", 4243);

    // Allow time for binding/connection (async)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Client sends message to server
    client.sendNetworkMessage("TestTopic", "HelloServer");

    // Allow time for transmission
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check if server received it
    auto serverMessages = server.getAllMessages();
    bool received = false;
    uint32_t clientId = 0;
    for (const auto& msg : serverMessages) {
        if (msg.topic == "TestTopic" && msg.payload == "HelloServer") {
            received = true;
            clientId = msg.clientId;
            break;
        }
    }
    EXPECT_TRUE(received);
    EXPECT_GT(clientId, 0); // Client ID should be assigned

    // Verify client list on server
    auto connectedClients = server.getConnectedClients();
    EXPECT_EQ(connectedClients.size(), 1);
    if (!connectedClients.empty()) {
        EXPECT_EQ(connectedClients[0].id, clientId);
        EXPECT_TRUE(connectedClients[0].connected);
    }

    // Server sends back to specific client
    server.sendToClient(clientId, "ReplyTopic", "HelloClient");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check client mailbox
    auto clientMessages = client.getAllMessages();
    received = false;
    for (const auto& msg : clientMessages) {
        if (msg.topic == "ReplyTopic" && msg.payload == "HelloClient") {
            received = true;
            break;
        }
    }
    EXPECT_TRUE(received);

    // Server broadcasts
    server.broadcast("BroadcastTopic", "GlobalMessage");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check client mailbox again
    clientMessages = client.getAllMessages();
    received = false;
    for (const auto& msg : clientMessages) {
        if (msg.topic == "BroadcastTopic" && msg.payload == "GlobalMessage") {
            received = true;
            break;
        }
    }
    EXPECT_TRUE(received);

    server.cleanup();
    client.cleanup();
}

TEST_F(NetworkManagerTest, MessageQueueOperations) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5569", "tcp://127.0.0.1:5570");
    nm.init();
    
    // Initially queue should be empty
    auto msg = nm.getLastMessage();
    EXPECT_FALSE(msg.has_value());
    
    auto allMsgs = nm.getAllMessages();
    EXPECT_TRUE(allMsgs.empty());
    
    nm.cleanup();
}

TEST_F(NetworkManagerTest, SendToClientDoesNotCrashWithInvalidId) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5565", "tcp://127.0.0.1:5566");
    nm.init();
    nm.bind(4250);
    
    // Sending to non-existent client should not crash
    EXPECT_NO_THROW(nm.sendToClient(999, "TestTopic", "TestPayload"));
    
    nm.cleanup();
}

TEST_F(NetworkManagerTest, BroadcastDoesNotCrashWithNoClients) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5567", "tcp://127.0.0.1:5568");
    nm.init();
    nm.bind(4251);
    
    // Broadcast with no clients should not crash
    EXPECT_NO_THROW(nm.broadcast("TestTopic", "TestPayload"));
    
    nm.cleanup();
}