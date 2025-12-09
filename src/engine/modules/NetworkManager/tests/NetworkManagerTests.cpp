#include <gtest/gtest.h>
#include "../NetworkManager.hpp"

// Mock or real instantiation? 
// Since NetworkManager uses asio and system resources, a real instantiation test 
// is actually an integration test. But for unit testing the logic, we usually mock things.
// However, without a large refactoring to inject dependencies (socket, io_context), 
// we will test the public interface "black box" style.

class NetworkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(NetworkManagerTest, Instantiation) {
    // Just check if we can create and destroy it without crash
    EXPECT_NO_THROW({
        rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556");
    });
}

TEST_F(NetworkManagerTest, BindDoesNotCrash) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5557", "tcp://127.0.0.1:5558");
    nm.init();
    // We can't easily verify "Bind" success without mocking or checking internal state/logs
    // But we can verify it doesn't throw exceptions
    EXPECT_NO_THROW(nm.bind(4242));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, ConnectDoesNotCrash) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5559", "tcp://127.0.0.1:5560");
    nm.init();
    EXPECT_NO_THROW(nm.connect("127.0.0.1", 4242));
    nm.cleanup();
}

// ============ Multi-Client Tests ============

TEST_F(NetworkManagerTest, GetConnectedClientsEmptyInitially) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5561", "tcp://127.0.0.1:5562");
    nm.init();
    
    auto clients = nm.getConnectedClients();
    EXPECT_TRUE(clients.empty());
    
    nm.cleanup();
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
    
    // Sending to non-existent client should not crash (just fail silently or return false)
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
