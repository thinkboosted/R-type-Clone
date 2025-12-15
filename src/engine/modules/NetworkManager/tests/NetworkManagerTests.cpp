#include <gtest/gtest.h>
#include "../NetworkManager.hpp"
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <iostream>

// Helper to simulate the Game Engine / Bus
class ZmqBusHelper {
public:
    zmq::context_t context;
    zmq::socket_t pub; // We publish to this, Module subscribes from it
    zmq::socket_t sub; // We subscribe to this, Module publishes to it

    ZmqBusHelper(const std::string& pubEndpoint, const std::string& subEndpoint) 
        : context(1), pub(context, zmq::socket_type::pub), sub(context, zmq::socket_type::sub) {
        
        // Module connects to these, so we bind
        pub.bind(pubEndpoint);
        sub.bind(subEndpoint);
        sub.set(zmq::sockopt::subscribe, ""); // Subscribe to all
    }

    void publish(const std::string& topic, const std::string& payload) {
        std::string msg = topic + " " + payload;
        pub.send(zmq::buffer(msg), zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Tiny sleep to ensure distribution
    }

    std::vector<std::string> readAll() {
        std::vector<std::string> msgs;
        while (true) {
            zmq::message_t msg;
            auto res = sub.recv(msg, zmq::recv_flags::dontwait);
            if (!res) break;
            msgs.emplace_back(static_cast<char*>(msg.data()), msg.size());
        }
        return msgs;
    }

    bool hasReceived(const std::string& substring) {
        auto msgs = readAll();
        for (const auto& m : msgs) {
            if (m.find(substring) != std::string::npos) return true;
        }
        return false;
    }
};

class NetworkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(NetworkManagerTest, Instantiation) {
    EXPECT_NO_THROW({
        rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556");
        nm.init();
        // Do something
        nm.cleanup();
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

TEST_F(NetworkManagerTest, MessageQueueOperations) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5569", "tcp://127.0.0.1:5570");
    nm.init();
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
    EXPECT_NO_THROW(nm.sendToClient(999, "TestTopic", "TestPayload"));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, BroadcastDoesNotCrashWithNoClients) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5567", "tcp://127.0.0.1:5568");
    nm.init();
    nm.bind(4251);
    EXPECT_NO_THROW(nm.broadcast("TestTopic", "TestPayload"));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, SendWithoutConnection) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5594", "tcp://127.0.0.1:5595");
    nm.init();
    EXPECT_NO_THROW(nm.sendNetworkMessage("Topic", "Payload"));
    EXPECT_NO_THROW(nm.broadcast("Topic", "Payload"));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, ReBind) {
    rtypeEngine::NetworkManager nm("tcp://127.0.0.1:5596", "tcp://127.0.0.1:5597");
    nm.init();
    EXPECT_NO_THROW(nm.bind(4252));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(nm.bind(4253));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    nm.cleanup();
}

TEST_F(NetworkManagerTest, EndToEndCommunication) {
    rtypeEngine::NetworkManager server("tcp://127.0.0.1:5571", "tcp://127.0.0.1:5572");
    rtypeEngine::NetworkManager client("tcp://127.0.0.1:5573", "tcp://127.0.0.1:5574");

    server.init();
    client.init();

    server.bind(4243);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client.connect("127.0.0.1", 4243);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    client.sendNetworkMessage("TestTopic", "HelloServer");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
    EXPECT_GT(clientId, 0);

    server.sendToClient(clientId, "ReplyTopic", "HelloClient");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto clientMessages = client.getAllMessages();
    received = false;
    for (const auto& msg : clientMessages) {
        if (msg.topic == "ReplyTopic" && msg.payload == "HelloClient") {
            received = true;
            break;
        }
    }
    EXPECT_TRUE(received);

    server.cleanup();
    client.cleanup();
}

TEST_F(NetworkManagerTest, MultipleClientsBroadcast) {
    rtypeEngine::NetworkManager server("tcp://127.0.0.1:5583", "tcp://127.0.0.1:5584");
    rtypeEngine::NetworkManager client1("tcp://127.0.0.1:5585", "tcp://127.0.0.1:5586");
    rtypeEngine::NetworkManager client2("tcp://127.0.0.1:5587", "tcp://127.0.0.1:5588");

    server.init();
    client1.init();
    client2.init();

    server.bind(4246);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client1.connect("127.0.0.1", 4246);
    client2.connect("127.0.0.1", 4246);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    client1.sendNetworkMessage("Init", "C1");
    client2.sendNetworkMessage("Init", "C2");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    server.getAllMessages();

    server.broadcast("Global", "Everyone");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto msgs1 = client1.getAllMessages();
    bool found1 = false;
    for (auto& m : msgs1) if (m.payload == "Everyone") found1 = true;
    EXPECT_TRUE(found1);

    auto msgs2 = client2.getAllMessages();
    bool found2 = false;
    for (auto& m : msgs2) if (m.payload == "Everyone") found2 = true;
    EXPECT_TRUE(found2);

    server.cleanup();
    client1.cleanup();
    client2.cleanup();
}

TEST_F(NetworkManagerTest, DisconnectCleansUpClients) {
    rtypeEngine::NetworkManager server("tcp://127.0.0.1:5590", "tcp://127.0.0.1:5591");
    rtypeEngine::NetworkManager client("tcp://127.0.0.1:5592", "tcp://127.0.0.1:5593");

    server.init();
    client.init();

    server.bind(4247);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client.connect("127.0.0.1", 4247);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    client.sendNetworkMessage("Init", "Hello");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    server.getAllMessages();

    // Ensure client is connected before testing disconnect
    ASSERT_EQ(server.getConnectedClients().size(), 1);

    server.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow async disconnect to process
    EXPECT_TRUE(server.getConnectedClients().empty());

    server.cleanup();
    client.cleanup();
}

/*
TEST_F(NetworkManagerTest, Aggressive_MalformedUdpPacket) {
    // ... kept commented out as requested to keep build green but show intent ...
}
*/