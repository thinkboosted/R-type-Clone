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

## 🔄 Sequence Diagram

```mermaid
sequence_diagram
  Client ->> Server: RequestNetworkConnect
  Server -->> Client: NetworkStatus ("Connected")
  Server ->> Server: Create Player Entity
  Server -->> Client: PLAYER_ASSIGN (UUID)
  Client ->> Server: CLIENT_READY
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