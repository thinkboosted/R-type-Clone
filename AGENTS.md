# 🤖 Agent Guide — R-Type Clone

AI assistant. Full read/write access: R-Type Clone repository (C++17, Lua/Sol2, ECS, UDP Networking, OpenGL, ZeroMQ).

<posture_and_communication>

- **Terse & Direct**: No politeness ("sure", "understand", "excellent"). Go straight to point.
- **Short fragments**: Max 1-2 sentences per response unless code/deep analysis. No transition sentences.
- **Ultra-Caveman**: Compress output. Strip articles/determiners ("the", "a", "an", "le", "la", "les") to save tokens.
- **Single solution**: No multiple choice. Propose best path. Ask clear question if uncertain.
- **No tool call with question**: If asking question/confirmation, stop. No tool call in same turn.
- **Zero Complacency**: Do not validate blindly. Point out flaws in Mael's reasoning/solutions before execution. Never pivot opinion because Mael insists. Explain past contradictions. Short/honest > long/complaisant.
- **Mandatory Verification**: Do not assume/hallucinate. Search files/tokensave first. Prefix unverified claims with "Unverified:".
- **Reactive Documentation**: Update documentation, code guidelines, sitemaps, and rules files immediately after design decisions/code changes.
  </posture_and_communication>

<tool_and_context_rules>

- **Prefer tokensave**: Before reading source files or scanning the codebase, use the tokensave MCP tools. They provide instant semantic results.
- **SQLite Database**: Query the SQLite database directly at `.tokensave/tokensave.db` (tables: `nodes`, `edges`, `files`) using SQL to answer complex structural queries.
- **No recursive grep**: Avoid heavy recursive scans or grep when tokensave can be used.
  - </tool_and_context_rules>

<development_workflow>

- **Phase 0 (COMPRENDRE)**: Use tokensave, explore key files, identify existing patterns before coding.
- **Phase 1 (PLANIFIER)**: Write 3-5 step plan with precise file paths.
- **Phase 2 (EXÉCUTER)**: Step-by-step implementation. Use targeted patches (`replace_file_content` / `multi_replace_file_content`). Check build/compiler feedback instantly.
- **Phase 3 (VÉRIFIER)**: Run static analysis, build the code (`python3 build.py`), and test both client and server modes to guarantee no regressions.
  </development_workflow>

<error_handling>

- **Tool Failure**: If tool fails twice, switch to fallback (e.g., if tokensave fails, use `grep_search`; if `grep_search` fails, search files/folders).
  </error_handling>

<budget_limits>

- **Tool loop breaker**: Max 5 consecutive tool calls without user. If stalled, stop, explain, ask Mael.
- **Token budget**: Avoid full reads for files >300 lines unless target lines specified. Use tokensave or read targeted ranges.
  </budget_limits>

<yagni_ladder>

1. Existing CLI/script.
2. Lightweight stdlib/package.
3. Project pattern.
4. Custom code (last resort).
   </yagni_ladder>

<loop_engineering>

- **Maker-Checker**: Code -> Test -> Verify -> Correct (max 10 inner loops).
- **Adversarial Check**: Auto-critique before presenting.
  </loop_engineering>

---

## 📋 Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture Summary](#2-architecture-summary)
3. [Directory Structure](#3-directory-structure)
4. [Module System](#4-module-system)
5. [ECS Architecture](#5-ecs-architecture)
6. [Networking Architecture](#6-networking-architecture)
7. [Lua Scripting Layer](#7-lua-scripting-layer)
8. [Build System](#8-build-system)
9. [Code Conventions](#9-code-conventions)
10. [Safe Contribution Guidelines](#10-safe-contribution-guidelines)
11. [Common Pitfalls](#11-common-pitfalls)
12. [Quick Reference](#12-quick-reference)

---

## 1. Project Overview

| Attribute              | Value                                           |
| :--------------------- | :---------------------------------------------- |
| **Language**     | C++17                                           |
| **Project Type** | Recreation of the classic arcade game*R-Type* |
| **Context**      | School project (EPITECH-style)                  |
| **Architecture** | Entity Component System (ECS)                   |
| **Networking**   | Client/Server multiplayer (UDP)                 |
| **Scripting**    | Lua (via Sol2)                                  |
| **Platforms**    | Linux, Windows                                  |

### 1.1 Project Goals

- Recreate the core gameplay of R-Type (side-scrolling space shooter)
- Implement a modular game engine with hot-swappable components
- Support both **solo** and **multiplayer** modes with parity
- Use a **server-authoritative** networking model for fairness

### 1.2 Technical Philosophy

- **Modularity:** Each subsystem (rendering, physics, networking, ECS) is an independent dynamic library
- **Decoupling:** Modules communicate exclusively via ZeroMQ pub/sub messaging
- **Scriptability:** Game logic lives in Lua scripts; C++ provides the engine infrastructure
- **Cross-platform:** Build system supports both Linux and Windows

---

## 2. Architecture Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              APPLICATION HOST                                │
│  (RTypeClient / RTypeServer)                                                │
│  - Loads modules dynamically                                                │
│  - Wires ZeroMQ endpoints                                                   │
│  - Manages lifecycle                                                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MODULE MANAGER                                     │
│  - dlopen/LoadLibrary for shared libraries                                  │
│  - createModule() C-style factory                                           │
│  - Module lifecycle: init() → loop() → cleanup()                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          ▼                           ▼                           ▼
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│  WindowManager  │       │    Renderer     │       │   LuaECSManager │
│  (SFML)         │       │  (GLEW/SFML)    │       │   (Sol2/Lua)    │
│                 │       │                 │       │                 │
│  - Window events│       │  - 3D/2D render │       │  - ECS logic    │
│  - Display      │       │  - Particles    │       │  - Scripts      │
└─────────────────┘       └─────────────────┘       └─────────────────┘
          │                           │                           │
          └───────────────────────────┼───────────────────────────┘
                                      ▼
                    ┌─────────────────────────────────┐
                    │      ZeroMQ Message Bus         │
                    │  (Topic-based Pub/Sub)          │
                    │  - Async, non-blocking          │
                    │  - String payloads or MsgPack   │
                    └─────────────────────────────────┘
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          ▼                           ▼                           ▼
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│  PhysicEngine   │       │ NetworkManager  │       │  SoundManager   │
│  (Bullet3)      │       │  (ASIO/UDP)     │       │  (SFML Audio)   │
│                 │       │                 │       │                 │
│  - Rigid bodies │       │  - UDP sockets  │       │  - SFX/Music    │
│  - Collisions   │       │  - MsgPack      │       │  - Playback     │
└─────────────────┘       └─────────────────┘       └─────────────────┘
```

### 2.1 Key Architectural Decisions

| Decision                     | Rationale                                                                      |
| :--------------------------- | :----------------------------------------------------------------------------- |
| **Dynamic modules**    | Allows independent development, replacement, and testing of subsystems         |
| **ZeroMQ pub/sub**     | Lightweight, transport-agnostic messaging; scales from in-process to networked |
| **Per-module threads** | Prevents slow subsystems from blocking others                                  |
| **C-style factory**    | Avoids C++ ABI mismatches between compiler versions                            |
| **Lua scripting**      | Enables rapid iteration on game logic without recompilation                    |

---

## 3. Directory Structure

```
R-type-Clone/
├── AGENTS.md                   # ← You are here
├── build.py                    # Python build script
├── CMakeLists.txt              # Root CMake configuration
├── CMakePresets.json           # CMake presets for Linux/Windows
├── vcpkg.json                  # Dependency manifest
│
├── assets/                     # Game assets (copied to build/)
│   ├── fonts/                  # TTF fonts
│   ├── models/                 # 3D models (.obj)
│   ├── scripts/                # Lua game logic
│   │   └── space-shooter/      # Main game scripts
│   │       ├── Main.lua        # Entry point
│   │       ├── GameLoop.lua    # Unified game loop
│   │       ├── components/     # Component definitions
│   │       ├── systems/        # ECS systems
│   │       └── levels/         # Level definitions
│   ├── sounds/                 # Audio files
│   └── textures/               # Texture images
│
├── cmake/
│   └── Dependencies.cmake      # FetchContent fallback for deps
│
├── docs/                       # Documentation
│   ├── CONTRIBUTING.md         # Contribution guidelines
│   ├── NETWORK_PROTOCOL.md     # Network protocol spec
│   ├── NetworkArchitecture.md  # Network system design
│   ├── OVERVIEW.md             # Project overview
│   ├── QUICKSTART.md           # Build instructions
│   └── rfc/                    # Request for Comments
│
├── src/
│   ├── engine/                 # Engine infrastructure
│   │   ├── app/                # Application base classes
│   │   ├── modules/            # Module implementations
│   │   │   ├── ECSManager/     # ECS module (LuaECSManager)
│   │   │   ├── NetworkManager/ # UDP networking
│   │   │   ├── PhysicEngine/   # Bullet physics
│   │   │   ├── Renderer/       # OpenGL rendering
│   │   │   ├── SoundManager/   # Audio playback
│   │   │   └── WindowManager/  # Window management
│   │   ├── modulesManager/     # Dynamic module loading
│   │   └── types/              # Shared type definitions
│   │
│   └── game/                   # Game-specific code
│       ├── Rtype.cpp/hpp       # Base game class
│       ├── client/             # Client application
│       └── server/             # Server application
│
└── vcpkg/                      # vcpkg submodule (dependencies)
```

---

## 4. Module System

### 4.1 Module Interface

All modules implement `rtypeEngine::IModule`:

```cpp
class IModule {
public:
    virtual void init() = 0;      // One-time initialization
    virtual void loop() = 0;      // Called every frame
    virtual void cleanup() = 0;   // Cleanup before unload
    virtual void start() = 0;     // Start module thread
    virtual void stop() = 0;      // Stop module thread

    // Messaging
    virtual void sendMessage(const std::string& topic, const std::string& message) = 0;
    virtual void subscribe(const std::string& topic, MessageHandler handler) = 0;
};
```

### 4.2 Module Loading

Modules are loaded dynamically via `AModulesManager`:

```cpp
// Each module exports this C-style factory function
extern "C" IModule* createModule(const char* pubEndpoint, const char* subEndpoint);
```

### 4.3 Available Modules

| Module                   | Library                     | Purpose                       |
| :----------------------- | :-------------------------- | :---------------------------- |
| `SFMLWindowManager`    | `SFMLWindowManager.so`    | Window creation, input events |
| `GLEWSFMLRenderer`     | `GLEWSFMLRenderer.so`     | OpenGL 3D/2D rendering        |
| `LuaECSManager`        | `LuaECSManager.so`        | ECS logic, Lua scripting      |
| `BulletPhysicEngine`   | `BulletPhysicEngine.so`   | Physics simulation            |
| `NetworkManager`       | `NetworkManager.so`       | UDP client/server networking  |
| `SFMLSoundManager`     | `SFMLSoundManager.so`     | Audio playback                |
| `BasicECSSavesManager` | `BasicECSSavesManager.so` | Save/load game state          |

### 4.4 Inter-Module Communication

Modules communicate via ZeroMQ pub/sub:

```cpp
// Publishing a message
sendMessage("RenderEntityCommand", "CreateEntity:cube:entity_123");

// Subscribing to messages
subscribe("KeyPressed", [](const std::string& key) {
    // Handle key press
});
```

**Common Topics:**
| Topic | Direction | Purpose |
| :--- | :--- | :--- |
| `RenderEntityCommand` | ECS → Renderer | Create/update/destroy render objects |
| `PhysicCommand` | ECS → Physics | Create/control physics bodies |
| `KeyPressed` / `KeyReleased` | Window → ECS | Input events |
| `EntityUpdated` | Physics → ECS | Physics simulation results |
| `NetworkMessage` | Network → ECS | Incoming network data |
| `RequestNetworkSend` | ECS → Network | Outgoing network data |
| `LoadScript` | App → ECS | Load Lua script file |
----------------------------------------

## 5. ECS Architecture

### 5.1 Overview

The ECS (Entity Component System) is implemented in Lua, managed by `LuaECSManager`:

- **Entities:** UUID strings (e.g., `"a1b2c3d4-..."`)
- **Components:** Lua tables with data
- **Systems:** Lua tables with `init()` and `update(dt)` functions

### 5.2 Component Pools

Components are stored in **sparse sets** for cache-efficient iteration:

```cpp
struct ComponentPool {
    std::vector<sol::table> dense;           // Component data
    std::vector<std::string> entities;       // Entity IDs (parallel to dense)
    std::unordered_map<std::string, size_t> sparse;  // Entity ID → index
};
```

### 5.3 Core Components

Defined in `assets/scripts/space-shooter/components/Components.lua`:

| Component           | Purpose                     |
| :------------------ | :-------------------------- |
| `Transform`       | Position, rotation, scale   |
| `Mesh`            | 3D model path, texture path |
| `Sprite`          | 2D texture reference        |
| `Collider`        | Physics collision shape     |
| `Physic`          | Mass, friction, velocity    |
| `Player`          | Player speed, properties    |
| `Enemy`           | Enemy AI properties         |
| `Bullet`          | Projectile damage           |
| `Life`            | Health/HP                   |
| `InputState`      | Current input state         |
| `NetworkIdentity` | Network ownership           |
| `GameState`       | Game state machine          |

### 5.4 ECS API (Lua)

```lua
-- Entity Management
local id = ECS.createEntity()
ECS.destroyEntity(id)

-- Components
ECS.addComponent(id, "Transform", Transform(0, 0, 0))
local t = ECS.getComponent(id, "Transform")
ECS.removeComponent(id, "Transform")
local has = ECS.hasComponent(id, "Transform")

-- Queries
local entities = ECS.getEntitiesWith({"Transform", "Player"})

-- Systems
ECS.registerSystem(MySystem)

-- Messaging
ECS.sendMessage("Topic", "payload")
ECS.subscribe("Topic", function(data) ... end)

-- Network (Binary Protocol)
ECS.sendBinary("INPUT", {k="UP", s=1})
ECS.sendNetworkMessage("TOPIC", "payload")
ECS.broadcastNetworkMessage("TOPIC", "payload")
```

### 5.5 System Loading Order

Systems are loaded in dependency order in `GameLoop.lua`:

1. **Core Gameplay** (Authority: Server + Solo)

   - `CollisionSystem`, `PhysicSystem`, `LifeSystem`, `EnemySystem`, `PlayerSystem`, `GameStateManager`
2. **Input & Control** (All instances)

   - `InputSystem`
3. **Network Sync** (Server + Client in Multiplayer)

   - `NetworkSystem`
4. **Menu & State** (All instances)

   - `MenuSystem`
5. **Visual & UI** (Client + Solo only)

   - `RenderSystem`, `ParticleSystem`, `ScoreSystem`, `BackgroundSystem`

---

## 6. Networking Architecture

### 6.1 Protocol Overview

| Feature                 | Specification        |
| :---------------------- | :------------------- |
| **Transport**     | UDP (via ASIO)       |
| **Serialization** | MsgPack (binary)     |
| **Architecture**  | Server-Authoritative |
| **Update Rate**   | 20Hz (50ms)          |

### 6.2 Capability Flags

The unified `ECS.capabilities` table determines behavior:

```lua
ECS.capabilities = {
    hasAuthority = true/false,      -- Can simulate game state
    hasRendering = true/false,      -- Can render graphics
    hasNetworkSync = true/false,    -- Network sync enabled
    hasLocalInput = true/false,     -- Accepts local input
    isServer = true/false,          -- Is server instance
    isLocalMode = true/false,       -- Solo mode
    isClientMode = true/false       -- Network client
}
```

**Mode Matrix:**

| Mode             | Authority | Rendering | NetworkSync | LocalInput |
| :--------------- | :-------: | :-------: | :---------: | :--------: |
| **Solo**   |    ✅    |    ✅    |     ❌     |     ✅     |
| **Server** |    ✅    |    ❌    |     ✅     |     ❌     |
| **Client** |    ❌    |    ✅    |     ✅     |     ✅     |

### 6.3 Core Network Messages

| Topic              | Direction         | Payload                            |
| :----------------- | :---------------- | :--------------------------------- |
| `INPUT`          | Client → Server  | `{k: "UP", s: 1}` (MsgPack)      |
| `ENTITY_POS`     | Server → Clients | Position, rotation, velocity, type |
| `PLAYER_ASSIGN`  | Server → Client  | Entity UUID                        |
| `CLIENT_READY`   | Client → Server  | Confirmation                       |
| `ENTITY_DESTROY` | Server → Clients | Entity UUID                        |

### 6.4 Wire Format

```
[ Topic Length (4 bytes) ] [ Topic String (N bytes) ] [ MsgPack Payload (M bytes) ]
```

---

## 7. Lua Scripting Layer

### 7.1 Script Entry Points

- **Solo/Client:** `assets/scripts/space-shooter/Main.lua` → Level loader
- **Unified Logic:** `assets/scripts/space-shooter/GameLoop.lua` → System loader

### 7.2 Writing Systems

```lua
local MySystem = {}

function MySystem.init()
    print("[MySystem] Initialized")
    -- Subscribe to events
    ECS.subscribe("MyEvent", MySystem.onMyEvent)
end

function MySystem.update(dt)
    -- Skip if not relevant to this instance
    if not ECS.capabilities.hasAuthority then return end

    -- Query entities
    local entities = ECS.getEntitiesWith({"MyComponent"})
    for _, id in ipairs(entities) do
        local comp = ECS.getComponent(id, "MyComponent")
        -- Update logic
    end
end

function MySystem.onMyEvent(data)
    -- Handle event
end

-- Register system
ECS.registerSystem(MySystem)
return MySystem
```

### 7.3 Capability Guards

**CRITICAL:** Always guard system logic based on capabilities:

```lua
-- Authority-only logic (physics, spawning, game rules)
if not ECS.capabilities.hasAuthority then return end

-- Rendering-only logic (visual effects, UI)
if not ECS.capabilities.hasRendering then return end

-- Input-only logic (keyboard/mouse handling)
if not ECS.capabilities.hasLocalInput then return end

-- Network-only logic (sync messages)
if not ECS.capabilities.hasNetworkSync then return end
```

---

## 8. Build System

### 8.1 Quick Build (Python Script)

```bash
python3 build.py
```

This script:

1. Detects/bootstraps vcpkg
2. Installs dependencies
3. Configures CMake
4. Builds the project
5. Copies assets to build directory

### 8.2 Manual Build (Linux)

```bash
# Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install

# Configure and build
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . -j$(nproc)
```

### 8.3 Running

```bash
cd build

# Solo mode (client only)
./r-type_client

# Multiplayer
./r-type_server 1234           # Start server on port 1234
./r-type_client 127.0.0.1 1234 # Connect to server

# Note: May need to set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD:$PWD/lib:$LD_LIBRARY_PATH
```

### 8.4 Dependencies (vcpkg.json)

- `sfml` - Window, graphics, audio
- `glew` - OpenGL extension loading
- `opengl` - Graphics API
- `zeromq` / `cppzmq` - Message bus
- `lua` / `sol2` - Scripting
- `bullet3` - Physics engine
- `asio` - Async networking
- `msgpack` - Binary serialization
- `gtest` - Unit testing

---

## 9. Code Conventions

### 9.1 C++ Style

- **Standard:** C++17
- **Naming:**
  - Classes: `PascalCase`
  - Functions/methods: `camelCase`
  - Private members: `_prefixedCamelCase`
  - Constants: `UPPER_SNAKE_CASE`
- **Namespaces:** `rtypeEngine` (engine), `rtypeGame` (game-specific)

### 9.2 Lua Style

- **Naming:**
  - Systems: `PascalCase`
  - Functions: `camelCase` or `PascalCase` for system methods
  - Components: `PascalCase` factory functions
  - Variables: `camelCase`
- **Indentation:** 4 spaces

### 9.3 Commit Messages (Conventional Commits)

```
<type>(<scope>): <short summary>
Types: feat, fix, docs, style, refactor, perf, test, chore, build, ci
Scope: renderer, lua, network, physics, build, etc.
Examples:
feat(renderer): add particle system support
fix(network): prevent packet loss on high latency
docs: update agent.md with networking section
```

---

## 10. Safe Contribution Guidelines

### 10.1 Before Making Changes

1. **Read this document** thoroughly
2. **Understand the scope** — Is this engine code, game logic, or configuration?
3. **Check capability guards** — Will this code run in the correct mode?
4. **Review related systems** — What other code depends on this?

### 10.2 Safe Changes (Low Risk)

- Adding new components in `Components.lua`
- Creating new systems that follow existing patterns
- Adding new assets (textures, sounds, models)
- Documentation updates
- Bug fixes with clear reproduction steps

### 10.3 Risky Changes (High Risk)

- Modifying module interfaces (`IModule`, `AModule`)
- Changing ZeroMQ message formats/topics
- Altering the ECS core (`LuaECSManager.cpp`)
- Modifying network protocol
- Changing capability flag behavior

### 10.4 Forbidden Actions

> ⚠️ **DO NOT:**
>
> - Invent new features without explicit request
> - Remove capability guards without understanding consequences
> - Change wire protocol without updating all consumers
> - Modify build system without testing both platforms
> - Add blocking calls in module loops
> - Use global state in Lua without documenting it

### 10.5 Testing Changes

1. **Solo Mode:** Run `./r-type_client` alone
2. **Multiplayer:** Run server + 2 clients
3. **Cross-platform:** Test on Linux AND Windows if modifying CMake

---

## 11. Common Pitfalls

### 11.1 Forgetting Capability Guards

```lua
-- ❌ WRONG: Runs everywhere, causes errors on server
function MySystem.update(dt)
    ECS.sendMessage("RenderEntityCommand", "...")  -- Server has no renderer!
end

-- ✅ CORRECT: Only runs where rendering exists
function MySystem.update(dt)
    if not ECS.capabilities.hasRendering then return end
    ECS.sendMessage("RenderEntityCommand", "...")
end
```

### 11.2 Blocking Module Loops

```cpp
// ❌ WRONG: Blocks the entire module
void MyModule::loop() {
    std::this_thread::sleep_for(std::chrono::seconds(5));  // BLOCKS!
}

// ✅ CORRECT: Non-blocking with state tracking
void MyModule::loop() {
    if (shouldDoAction()) {
        doAction();
        _lastActionTime = now();
    }
}
```

### 11.3 Hardcoded Network Assumptions

```lua
-- ❌ WRONG: Assumes multiplayer mode
function MySystem.init()
    ECS.subscribe("NetworkMessage", ...)  -- Fails in solo mode!
end

-- ✅ CORRECT: Check capability first
function MySystem.init()
    if ECS.capabilities.hasNetworkSync then
        ECS.subscribe("NetworkMessage", ...)
    end
end
```

### 11.4 Missing Entity Validation

```lua
-- ❌ WRONG: May crash if entity was destroyed
local t = ECS.getComponent(id, "Transform")
t.x = t.x + 1  -- Crash if t is nil!

-- ✅ CORRECT: Always validate
local t = ECS.getComponent(id, "Transform")
if t then
    t.x = t.x + 1
end
```

---

## 12. Quick Reference

### 12.1 File Locations

| What                | Where                                                      |
| :------------------ | :--------------------------------------------------------- |
| Game entry (client) | `src/game/client/main.cpp`                               |
| Game entry (server) | `src/game/server/main.cpp`                               |
| Module base class   | `src/engine/modules/AModule.hpp`                         |
| ECS manager         | `src/engine/modules/ECSManager/LuaECSManager/`           |
| Game scripts        | `assets/scripts/space-shooter/`                          |
| Component defs      | `assets/scripts/space-shooter/components/Components.lua` |
| System scripts      | `assets/scripts/space-shooter/systems/`                  |
| Build config        | `CMakeLists.txt`, `vcpkg.json`                         |

### 12.2 Key Commands

```bash
# Build
python3 build.py

# Run solo
cd build && ./r-type_client

# Run multiplayer
cd build
./r-type_server 1234 &
./r-type_client 127.0.0.1 1234

# Clean rebuild
rm -rf build && python3 build.py
```

### 12.3 Useful Debug Messages

In Lua scripts:

```lua
print("[SystemName] Debug message")
print("[SystemName] Capabilities: Authority=" .. tostring(ECS.capabilities.hasAuthority))
```

In C++ modules:

```cpp
std::cout << "[ModuleName] Debug message" << std::endl;
```

---

## 📚 Additional Documentation

- [QUICKSTART.md](docs/QUICKSTART.md) — Build instructions
- [OVERVIEW.md](docs/OVERVIEW.md) — Engine architecture details
- [NetworkArchitecture.md](docs/NetworkArchitecture.md) — Network system design
- [NETWORK_PROTOCOL.md](docs/NETWORK_PROTOCOL.md) — Wire protocol specification
- [CONTRIBUTING.md](docs/CONTRIBUTING.md) — Contribution guidelines
- [RFC-001-BINARY-PROTOCOL.md](docs/rfc/RFC-001-BINARY-PROTOCOL.md) — Binary protocol RFC

---

*Last updated: July 1, 2026*
