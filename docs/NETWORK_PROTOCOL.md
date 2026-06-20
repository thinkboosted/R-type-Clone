# 📡 R-Type Network Protocol (Binary Edition)

## 📋 Overview
This document defines the **Binary Protocol** used for Client-Server communication in the R-Type Clone. It relies on **UDP** for transport and **MsgPack** for efficient data serialization.

| Feature | Specification |
| :--- | :--- |
| **Transport** | UDP (via ASIO) |
| **Serialization** | MsgPack (Binary) |
| **Architecture** | Server-Authoritative |
| **Update Rate** | 20Hz (50ms) |

---

## 🛠 Internal Message Layout
Messages travelling through the engine's internal ZeroMQ bus use a specific binary packing to avoid string parsing overhead and corruption.

```text
[ Topic Length (4 bytes) ] [ Topic String (N bytes) ] [ MsgPack Payload (M bytes) ]
```

---

## 📨 Core Messages

### 1. `INPUT` (Client ➔ Server)
Sent whenever a key is pressed or released.

**Topic:** `INPUT`  
**Payload (MsgPack Map):**
| Key | Type | Description |
| :-- | :--- | :--- |
| `k` | String | Key name (`UP`, `DOWN`, `LEFT`, `RIGHT`, `SPACE`) |
| `s` | Integer | State: `1` (Pressed), `0` (Released) |

**Lua Example:**
```lua
ECS.sendBinary("INPUT", {k="UP", s=1})
```

---

### 2. `ENTITY_POS` (Server ➔ Clients)
Authoritative state broadcast for every moving entity.

**Topic:** `ENTITY_POS`  
**Payload (MsgPack Map):**
| Key | Type | Description |
| :-- | :--- | :--- |
| `id` | String | Unique Entity UUID |
| `x`, `y`, `z` | Float | World Position |
| `rx`, `ry`, `rz` | Float | World Rotation (Degrees) |
| `vx`, `vy`, `vz` | Float | Velocity (for Client Extrapolation) |
| `t` | Integer | Entity Type (1:Player, 2:Bullet, 3:Enemy) |

---

### 3. `PLAYER_ASSIGN` (Server ➔ Client)
Assigns a unique ID and entity to a newly connected client.

**Topic:** `PLAYER_ASSIGN`  
**Payload:** String (Entity ID)

---

### 4. `PLAYER_JOIN` (Client ➔ Server)
Signals that a connected client entered the multiplayer lobby.

**Topic:** `PLAYER_JOIN`  
**Payload:** String (free text, usually `join`)

---

### 5. `PLAYER_READY` (Client ➔ Server)
Marks a lobby player as ready.

**Topic:** `PLAYER_READY`  
**Payload:** String (free text, usually `ready`)

---

### 6. `GAME_START` (Server ➔ Clients)
Broadcast when all connected lobby players are ready.

**Topic:** `GAME_START`  
**Payload:** String (`all_ready`)

---

### 7. `ACK` (Client ➔ Server)
Acknowledges reliable control messages.

**Topic:** `ACK`  
**Payload:** String (acked topic, e.g. `PLAYER_ASSIGN` or `GAME_START`)

---

## ✅ Reliable Control Messages

The server tracks a small set of critical messages and resends them until ACK:

- `PLAYER_ASSIGN`
- `GAME_START`

Resend policy:

- Interval: 500ms
- Max attempts: 6
- Timeout report: `NetworkError` publishes `ReliableMessageTimeout:<clientId>:<topic>`

Error handling behavior:

- Missing ACK triggers retransmission.
- Reaching max retries emits timeout telemetry (`NetworkError`) and drops the pending reliable packet.
- Client disconnect removes all pending reliable packets for that client.

---

## 🔄 Sequence Diagram

```mermaid
sequence_diagram
  Client ->> Server: RequestNetworkConnect
  Server -->> Client: NetworkStatus ("Connected")
  Client ->> Server: PLAYER_JOIN
  Client ->> Server: PLAYER_READY
  Server -->> Client: GAME_START
  Client ->> Server: ACK("GAME_START")
  Server ->> Server: Create Player Entity
  Server -->> Client: PLAYER_ASSIGN (UUID)
  Client ->> Server: ACK("PLAYER_ASSIGN")
  loop Gameplay
    Client ->> Server: INPUT (Binary MsgPack)
    Server ->> Server: Simulate Physics
    Server -->> Client: ENTITY_POS (Binary MsgPack)
    Client ->> Client: Interpolate & Render
  end
```

---

## 🚀 Performance Benefits
1. **Zero Parsing:** No `string.match` or `stringstream` needed.
2. **Compactness:** MsgPack reduces payload size by ~40% compared to JSON/Text.
3. **Safety:** Length-prefixed topics prevent buffer overflows and injection attacks.