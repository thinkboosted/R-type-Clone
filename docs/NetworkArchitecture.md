# 🌐 NetworkManager Architecture

![Status](https://img.shields.io/badge/Status-Prototype-yellow?style=for-the-badge)
![Protocol](https://img.shields.io/badge/Protocol-UDP-blue?style=for-the-badge)
![Lang](https://img.shields.io/badge/Language-C++17-red?style=for-the-badge)
![Lib](https://img.shields.io/badge/Library-Asio%20%7C%20MsgPack-green?style=for-the-badge)

The **NetworkManager** module is the backbone of the R-Type project communications. It ensures fast, asynchronous data transport between the Server and Clients.

---

## 📑 Table of Contents
1. [Why UDP?](#1-why-udp)
2. [System Architecture](#2-system-architecture)
3. [Data Flow](#3-data-flow)
4. [Technical Specifications](#4-technical-specifications)
5. [Roadmap & Limitations](#5-roadmap--limitations)

---

## 1. Why UDP?

In accordance with real-time video game standards and project specifications, we use **UDP** instead of TCP.

| Feature | TCP (Transmission Control Protocol) 🐢 | UDP (User Datagram Protocol) 🚀 |
| :--- | :--- | :--- |
| **Concept** | Phone Call ("Hello? Can you hear me?") | Sending Postcards (Fire & Forget) |
| **Guarantee** | **Reliable** (Order and delivery guaranteed) | **Unreliable** (Loss and disorder possible) |
| **Speed** | Slow (Handshake, ACK, congestion control) | **Maximum** (No waiting) |
| **R-Type Usage** | Chat, Login, Level Loading | **Movement, Shooting, Health, States** |

> [!IMPORTANT]
> **Real-Time Philosophy:** It is better to lose a packet containing the player's position from 50ms ago than to freeze the game to recover it. The next position (arriving in 16ms) is more important.

---

## 2. System Architecture

The system relies on a layered architecture to decouple game logic from pure networking.

```mermaid
graph TD
    subgraph "Game Engine Layer"
        GameLoop[Game Loop / ECS]
        GameLoop -- "RequestNetworkSend" --> ZMQ_PUB[ZeroMQ Publisher]
        ZMQ_SUB[ZeroMQ Subscriber] -- "NetworkMessage" --> GameLoop
    end

    subgraph "Network Module Layer"
        NetManager[NetworkManager Class]
        ZMQ_PUB -.-> NetManager
        NetManager -.-> ZMQ_SUB

        Serializer[MsgPack Serializer]
        Asio[Asio UDP Socket]

        NetManager <--> Serializer
        NetManager <--> Asio
    end

    subgraph "Physical Layer"
        Internet((Internet / LAN))
        Asio <--> Internet
    end
```

---

## 3. Data Flow

Here is the complete lifecycle of a packet, from emission to reception.

### 📤 Outbound (Sending)

```mermaid
sequenceDiagram
    participant Game as Game Logic
    participant Bus as ZeroMQ Bus
    participant NM as NetworkManager
    participant Net as UDP Socket

    Game->>Bus: Publish RequestNetworkSend Move 10 20
    Bus->>NM: Callback
    NM->>NM: Create NetworkEnvelope
    NM->>NM: Serialize MsgPack to binary
    NM->>Net: async_send_to
```

### 📥 Inbound (Receiving)

```mermaid
sequenceDiagram
    participant Net as UDP Socket
    participant NM as NetworkManager
    participant Bus as ZeroMQ Bus
    participant Game as Game Logic

    Net->>NM: async_receive_from
    NM->>NM: Deserialize MsgPack to NetworkEnvelope
    NM->>Bus: Publish NetworkMessage Move 10 20
    Bus->>Game: Callback
    Game->>Game: Process Move
```

---

## 4. Technical Specifications

### Packet Structure (Wire Format)
Thanks to UDP, a received packet is always complete. We use **MsgPack** to encode data.

| Component | Type | Description |
| :--- | :--- | :--- |
| **Topic** | `String` | The message subject (e.g., "PlayerMove", "GameStart"). |
| **Payload** | `String` | The raw data of the message (can be JSON, text, or binary). |

### ZeroMQ Commands (Internal API)

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `RequestNetworkBind` | `Port` (e.g., "4242") | Opens the server to listen on the given port. |
| `RequestNetworkConnect` | `IP Port` (e.g., "127.0.0.1 4242") | Connects the client to a target address. |
| `RequestNetworkSend` | `Topic Payload` | Sends a message to the last known target. |
| `NetworkStatus` | `Message` | (Event) Notifies of a state change (Bound, Connected). |

---


## 5. Roadmap & Limitations

The current implementation is a **v1.0 (Prototype)**. It is functional for 1v1 but requires evolution for the final version.

### 🚧 Current Limitations
*   **Single-Target (Last Speaker Rule):** The server only replies to the *last* client that spoke to it. There is no client list.
*   **Zero Reliability:** If a packet is lost, it is gone. No retransmission.

### 🗺️ Development Plan (Next Steps)

#### A. Multi-Client Management (High Priority)
- [ ] Create a `ClientSession` structure (ID, Endpoint, LastActivity).
- [ ] Maintain a `std::map<ClientID, ClientSession>` on the server side.
- [ ] Modify the API to target a player: `RequestNetworkSendTo "ClientID Payload"`.
- [ ] Add Broadcast: `RequestNetworkBroadcast "Payload"`.

#### B. Reliability Layer
For critical messages (*Game Start*, *Player Death*U), we cannot afford loss.
- [ ] Add a **Sequence ID** in the packet header.
- [ ] Implement an **ACK** (Acknowledgment) system.
- [ ] Create a buffer of "Messages waiting for ACK" to retransmit after `x` ms.

#### C. Connection Maintenance
- [ ] **Heartbeat:** Send a ping every second.
- [ ] **Timeout:** Disconnect a client silent for 5 seconds.

> [!TIP]
> For the reliability layer, no need to reinvent TCP. Look at the protocol used by *Quake 3* or the *Enet* library for inspiration.