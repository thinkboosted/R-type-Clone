# 📖 Project Overview

This document provides a high-level overview of the R-Type Clone project.

For detailed technical architecture, see [🏗️ Architecture](ARCHITECTURE.md).

---

## 🎯 Project Goals

- **Modular Engine**: Lightweight, modular game-engine prototype
- **Cross-Platform**: Runs on Linux and Windows
- **Dynamic Loading**: Modules loaded at runtime as shared libraries
- **Message-Driven**: Inter-module communication via ZeroMQ pub/sub

---

## 🏛️ Architecture Summary

### Application Host

The main process handles:
- Loading modules from shared libraries
- Wiring messaging endpoints
- Managing program lifecycle

### Module System

Each subsystem (window, renderer, ECS, physics, sound, network) is:
- An independent shared library
- Implementing `IModule` interface
- Running in its own thread
- Communicating via messages

### Communication

- **Pattern**: Topic-based publish/subscribe
- **Transport**: ZeroMQ sockets
- **Format**: String topics, string/binary payloads

---

## 🔧 Implemented Modules

| Module | Library | Purpose |
|--------|---------|---------|
| **WindowManager** | SFML | Window creation, input events |
| **Renderer** | OpenGL/GLEW | 3D rendering |
| **LuaECS** | Lua/Sol2 | Entity Component System |
| **PhysicEngine** | Bullet3 | Physics simulation |
| **SoundManager** | SFML | Audio playback |
| **NetworkManager** | Asio | Client-server networking |

---

## 📦 Dependencies

Managed via vcpkg:

- `sfml` - Window and audio
- `glew` - OpenGL extensions
- `zeromq` / `cppzmq` - Messaging
- `lua` / `sol2` - Scripting
- `bullet3` - Physics
- `asio` - Networking
- `msgpack` - Serialization
- `gtest` - Testing

---

## 📚 Further Reading

- [🏗️ Architecture](ARCHITECTURE.md) - Detailed technical architecture
- [📨 Channels](CHANNELS.md) - Message channel reference
- [📡 Network Protocol](NETWORK_PROTOCOL.md) - Network communication
- [📦 Installation](INSTALL.md) - Build instructions
- [🤝 Contributing](CONTRIBUTING.md) - How to contribute

