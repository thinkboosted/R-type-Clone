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
