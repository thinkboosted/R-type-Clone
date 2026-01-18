# 🏗️ Architecture Overview

This document describes the architecture of the R-Type Clone game engine.

---

## 📋 Table of Contents

- [High-Level Architecture](#high-level-architecture)
- [Module System](#-module-system)
- [Inter-Module Communication](#-inter-module-communication)
- [Threading Model](#-threading-model)
- [ECS Architecture](#-ecs-architecture)
- [Network Architecture](#-network-architecture)
- [Module Reference](#-module-reference)

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Application Host                             │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                      ModulesManager                            │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐         │  │
│  │  │  Window  │ │ Renderer │ │   ECS    │ │  Sound   │   ...   │  │
│  │  │ Manager  │ │  (GLEW)  │ │  (Lua)   │ │ Manager  │         │  │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘         │  │
│  │       │            │            │            │                │  │
│  │       └────────────┴────────────┴────────────┘                │  │
│  │                          │                                     │  │
│  │                ┌─────────┴─────────┐                          │  │
│  │                │    ZeroMQ Bus     │                          │  │
│  │                │    (Pub/Sub)      │                          │  │
│  │                └───────────────────┘                          │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 🔌 Module System

### Design Philosophy

The engine uses a **dynamic module architecture** where each subsystem is:

- Loaded at runtime as a shared library (`.so` / `.dll`)
- Isolated in its own thread
- Communicating via message passing
- Hot-swappable without recompiling the host

### Module Interface

All modules implement the `IModule` interface:

```cpp
namespace rtypeEngine {

class IModule {
public:
    virtual ~IModule() = default;
    
    // Lifecycle
    virtual void init() = 0;
    virtual void loop() = 0;
    virtual void stop() = 0;
    virtual void cleanup() = 0;
    virtual void release() = 0;
    
    // Messaging
    virtual void subscribe(const std::string& topic, MessageHandler handler) = 0;
    virtual void sendMessage(const std::string& topic, const std::string& message) = 0;
    
    // Threading
    virtual void start() = 0;
    virtual void processMessages() = 0;
};

}
```

### Module Loading

Modules are loaded via a C-style factory function to avoid ABI issues:

```cpp
// In each module's .cpp file
extern "C" {
    rtypeEngine::IModule* createModule(
        const std::string& pubEndpoint,
        const std::string& subEndpoint
    );
}
```

The `ModulesManager` handles:
1. Opening shared libraries (`dlopen`/`LoadLibrary`)
2. Retrieving the `createModule` symbol
3. Instantiating modules with messaging endpoints
4. Managing module lifecycle

---

## 📨 Inter-Module Communication

### ZeroMQ Pub/Sub Pattern

Modules communicate via a **topic-based publish/subscribe** system using ZeroMQ:

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   Module A  │         │  ZMQ Proxy  │         │   Module B  │
│             │         │             │         │             │
│  Publisher ─┼────────►│ XSUB  XPUB │◄────────┼─ Subscriber │
│  Subscriber◄┼─────────│             │─────────┼► Publisher  │
└─────────────┘         └─────────────┘         └─────────────┘
```

### Message Format

```
[ Topic (string) ] [ Payload (string/binary) ]
```

For binary messages:
```
[ Topic Length (4 bytes) ] [ Topic (N bytes) ] [ MsgPack Payload (M bytes) ]
```

### Example Communication Flow

```
LuaECS                    Renderer                 WindowManager
   │                         │                          │
   │  RenderEntityCommand    │                          │
   ├────────────────────────►│                          │
   │                         │                          │
   │                         │  (renders frame)         │
   │                         │                          │
   │                         │     ImageRendered        │
   │                         ├─────────────────────────►│
   │                         │                          │
   │                         │                    (displays)
```

---

## 🧵 Threading Model

### Per-Module Threading

Each module runs in its own thread:

```cpp
class AModule : public IModule {
protected:
    std::thread _moduleThread;
    std::atomic<bool> _running{false};
    
public:
    void start() override {
        _running = true;
        _moduleThread = std::thread([this]() {
            while (_running) {
                processMessages();
                loop();
            }
        });
    }
};
```

### Benefits

- **Isolation**: Slow modules don't block others
- **Parallelism**: Utilize multi-core CPUs
- **Resilience**: Module crashes are contained

### Synchronization

- Message queues are thread-safe (ZeroMQ handles this)
- Shared state is minimized
- Critical sections use mutexes when necessary

---

## 🎮 ECS Architecture

### Overview

The Entity Component System is implemented in Lua using Sol2:

```
┌─────────────────────────────────────────────────────────────┐
│                      LuaECSManager                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │   Entities   │  │  Components  │  │   Systems    │       │
│  │              │  │              │  │              │       │
│  │  ID: 1 ──────┼──┤ Transform    │  │ InputSystem  │       │
│  │  ID: 2 ──────┼──┤ Physic       │  │ PlayerSystem │       │
│  │  ID: 3 ──────┼──┤ Mesh         │  │ RenderSystem │       │
│  │  ...         │  │ ...          │  │ ...          │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

### Components

Components are Lua tables created by factory functions:

```lua
function Transform(x, y, z, rx, ry, rz, sx, sy, sz)
    return {
        x = x or 0, y = y or 0, z = z or 0,
        rx = rx or 0, ry = ry or 0, rz = rz or 0,
        sx = sx or 1, sy = sy or 1, sz = sz or 1
    }
end
```

### Systems

Systems process entities with specific components:

```lua
local RenderSystem = {}

function RenderSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "Mesh"})
    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local mesh = ECS.getComponent(id, "Mesh")
        -- Update rendering...
    end
end

ECS.registerSystem(RenderSystem)
```

### Capabilities System

The ECS supports different runtime modes:

```lua
ECS.capabilities = {
    hasAuthority = true,      -- Can modify game state
    hasRendering = true,      -- Has graphics output
    hasLocalInput = true,     -- Has local input
    hasNetworkSync = false,   -- Network synchronization
    isClientMode = false,     -- Running as client
    isServerMode = false      -- Running as server
}
```

---

## 🌐 Network Architecture

### Server-Authoritative Model

```
┌──────────────────┐                    ┌──────────────────┐
│      Server      │                    │      Client      │
│                  │                    │                  │
│  ┌────────────┐  │                    │  ┌────────────┐  │
│  │    ECS     │  │   ENTITY_POS       │  │    ECS     │  │
│  │ (Authority)│  ├───────────────────►│  │ (Predicted)│  │
│  └────────────┘  │                    │  └────────────┘  │
│                  │                    │                  │
│  ┌────────────┐  │      INPUT         │  ┌────────────┐  │
│  │  Physics   │  │◄───────────────────┤  │   Input    │  │
│  │ Simulation │  │                    │  │  Handler   │  │
│  └────────────┘  │                    │  └────────────┘  │
└──────────────────┘                    └──────────────────┘
```

### Message Flow

1. **Client sends input** → `INPUT` message to server
2. **Server processes** → Physics simulation, game logic
3. **Server broadcasts** → `ENTITY_POS` to all clients
4. **Client interpolates** → Smooth visual updates

### Protocol

- **Transport**: UDP via ASIO
- **Serialization**: MsgPack
- **Update Rate**: 20Hz (50ms intervals)

See [Network Protocol](NETWORK_PROTOCOL.md) for details.

---

## 📦 Module Reference

### WindowManager (SFML)

**Purpose**: Window creation and input handling

**Subscribes to**:
- `ImageRendered` - Display rendered frames

**Publishes**:
- `KeyPressed` - Key press events
- `KeyReleased` - Key release events
- `MousePressed` - Mouse click events
- `MouseMoved` - Mouse movement events
- `WindowResized` - Window resize events

### Renderer (GLEW/SFML)

**Purpose**: 3D rendering with OpenGL

**Subscribes to**:
- `RenderEntityCommand` - Entity rendering commands

**Publishes**:
- `ImageRendered` - Frame ready for display

### LuaECSManager

**Purpose**: Entity Component System with Lua scripting

**Subscribes to**:
- Various game events (forwarded to Lua systems)

**Publishes**:
- `RenderEntityCommand` - Rendering instructions
- `PhysicCommand` - Physics instructions
- Sound/Music commands

### PhysicEngine (Bullet3)

**Purpose**: Physics simulation

**Subscribes to**:
- `PhysicCommand` - Physics instructions

**Publishes**:
- `Collision` - Collision events

### SoundManager (SFML)

**Purpose**: Audio playback

**Subscribes to**:
- `SoundPlay` - Play sound effect
- `SoundStopAll` - Stop all sounds
- `MusicPlay` - Play music
- `MusicStop` - Stop music
- `MusicPause` / `MusicResume` - Control music

### NetworkManager

**Purpose**: Client-server communication

**Handles**:
- UDP socket management
- Client connections
- Message routing
- Binary protocol encoding/decoding

---

## 🔗 See Also

- [Message Channels](CHANNELS.md)
- [Network Protocol](NETWORK_PROTOCOL.md)
- [API Reference](api/index.html)
