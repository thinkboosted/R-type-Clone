# R-type-Clone — Project Overview

**Purpose:**
- **Goal:** Provide a lightweight, modular game-engine prototype designed to run on both Linux and Windows. The engine composes functionality as independent, dynamically-loaded modules (one module per dynamic library) that communicate over a global messaging canal.

**Architecture (high level):**
- **Application host:** The main process is responsible for loading modules, wiring their messaging endpoints, and owning the program lifecycle. Modules are loaded at runtime as shared libraries and instantiated through a C-style factory function (`createModule`).
- **Modules:** Each logical subsystem (window, renderer, Lua ECS, physics, etc.) is provided as a module. Modules implement a common interface (`rtypeEngine::IModule` / `AModule`) and run their own thread for isolated execution.
- **Module loader:** `rtypeEngine::AModulesManager` / `AModulesManager` handle opening shared libraries (`dlopen`/`LoadLibrary`), retrieving the `createModule` symbol and keeping module handles for lifecycle management.

**Inter-module communication:**
- **Messaging pattern:** Modules communicate via a global, topic-based publish/subscribe canal. The engine uses ZeroMQ (and the C++ wrappers) to implement pub/sub sockets. Modules call `sendMessage(topic, message)` and subscribe with `subscribe(topic, handler)`.
- **Message model:** Topics are strings; payloads are passed as `std::string`. Handlers are `std::function<void(const std::string&)>`, allowing flexible payload formats (JSON, custom binary encoded strings, etc.).
- **Endpoints:** Each module receives publisher and subscriber endpoints at construction (see `AModule` constructor parameters `pubEndpoint` and `subEndpoint`) so the module can bind/connect sockets consistently with the host wiring.

**Threading and lifecycle:**
- **Per-module thread:** `AModule` contains a `_moduleThread` and an `_running` atomic flag. Modules implement `start()`, `stop()` and `processMessages()` to manage their loop and message processing.
- **Lifecycle hooks:** `IModule` defines `init()`, `loop()`, `stop()`, `cleanup()`, and `release()` which modules implement to perform initialization, per-frame work, and cleanup.
- **Unloading:** `AModulesManager` clears shared pointers to module instances and then unloads library handles. Modules must ensure threads are stopped and resources released before the library is unloaded.

**Implemented modules (examples):**
- **WindowManager:** Manages the application window and subscribes to `ImageRendered` events to display renderer output.
- **Renderer:** Renders 3D scenes, subscribes to `SceneUpdated` events, and publishes `ImageRendered` events when new frames are ready.
- **LuaECS:** Hosts the entity-component system driven by Lua scripts using `sol2`/`lua`. It issues commands (e.g., `RenderEntityCommand`) as messages for other modules to act upon.
- **PhysicEngine:** Uses Bullet Physics to simulate rigid bodies, receives physics commands from `LuaECS` and emits events (collisions, triggers) back into the messaging system.

**Build system & dependencies:**
- **Build:** Cross-platform CMake-based build with a `CMakePresets.json` for convenience. The project supports building on Linux and Windows (MSVC) and integrates with `vcpkg` (manifest mode) for dependency management.
- **Key dependencies (vcpkg):** `sfml`, `glew`, `opengl`, `zeromq`, `cppzmq`, `lua`, `sol2`, `bullet3`. ZeroMQ and cppzmq are used for the messaging canal; Bullet3 for physics; `sol2` and `lua` for scripting.

**Technical choices & rationale:**
- **Dynamic modules (shared libs):** Decouples subsystems so each module can be developed, replaced, or restarted independently and keeps the host small.
- **ZeroMQ pub/sub:** Lightweight, language- and transport-agnostic message passing with topic filtering; it scales from in-process to remote processes which leaves room for future distribution.
- **Per-module threads:** Modules run independently to avoid blocking the entire engine on slow subsystems (e.g., script loading or heavy physics steps). Communication is asynchronous via messages.
- **C API factory (`createModule`):** Simplifies cross-platform loading of modules from shared libraries by avoiding C++ ABI mismatches between compiler versions.

**How it typically flows (example):**
- `LuaECS` script updates entity transforms → sends `SceneUpdated` message → `Renderer` receives it, re-renders → `Renderer` publishes `ImageRendered` → `WindowManager` subscribes and displays it.

**Where to look next:**
- `src/engine/modules/` — module base classes and per-module implementations.
- `src/engine/modulesManager/` — module loader and lifecycle management.
- `vcpkg.json` and `CMakeLists.txt` — dependency and build configuration.
- `docs/QUICKSTART.md` — build and run instructions.

**Next documentation tasks (planned):**
- Add a `MODULESYSTEM.md` covering module build targets, recommended compile flags, and the exact message wire format.
- Per-module READMEs describing their specific topics/messages and expected payload schemas (e.g., Renderer: `ImageRendered`, LuaECS: `RenderEntityCommand`, PhysicEngine: collision events).

If you want, I can now:
- Generate a `MODULESYSTEM.md` with precise wiring (endpoints, how to build a module) or
- Create per-module markdown stubs under `docs/modules/` to fill later.
