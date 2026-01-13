# RFC 001: Strict Binary Network Protocol

| Attribute | Value |
| :--- | :--- |
| **Status** | PROPOSED |
| **Author** | R-Type Core Team |
| **Created** | 2026-01-07 |
| **Category** | Networking |

## 1. Introduction
This RFC defines a strict binary communication protocol for the R-Type project. The goal is to move away from text-based messaging to fulfill the project's technical requirements and optimize real-time performance.

## 2. Motivation
Previous implementations used string concatenation (e.g., `"UP 1"`) which suffered from:
- **Parsing Overhead:** Heavy use of Regex/Match patterns.
- **Fragility:** Special characters or spaces in data could break the protocol.
- **Size:** Textual representation of floats is inefficient.

## 3. Specification

### 3.1 Serialization Format: MsgPack
We use **MessagePack (MsgPack)** as our primary serialization layer. 
- It provides JSON-like flexibility with a purely binary representation.
- Supported in both C++ (msgpack-cxx) and Lua (via custom binding).

### 3.2 Internal Transport Envelope
To ensure data integrity between the ZeroMQ bus and the Network Module, every message is packed as follows:

| Offset | Length | Name | Description |
| :--- | :--- | :--- | :--- |
| 0 | 4 bytes | `topic_len` | Unsigned 32-bit integer (Little Endian) |
| 4 | N bytes | `topic` | UTF-8 encoded topic string |
| 4 + N | M bytes | `payload` | Raw MsgPack binary data |

### 3.3 Security & Robustness
- **Topic Validation:** Topics exceeding 1024 characters are rejected.
- **Bounds Checking:** Every message handler verifies if the payload size matches the expected length before copying memory.
- **Type Safety:** MsgPack handles type conversion between Lua and C++, preventing malformed numeric inputs.

## 4. Drawbacks
- **Human Readability:** Network traffic is no longer readable via simple `print()` or standard sniffers without a MsgPack decoder.
- **Dependency:** Adds `msgpack-cxx` as a required library.

## 5. Alternatives Considered
- **Raw Structs:** Faster but lacks flexibility for dynamic Lua components.
- **Protobuf:** Robust but adds complex code generation steps that might complicate the current modular architecture.

## 6. Implementation Status
Successfully implemented in:
- `NetworkManager` (C++)
- `LuaECSManager` (C++)
- `NetworkSystem.lua`
- `InputSystem.lua`
