## R-Type Project To-Do List

### Continuous Check

| Category | Requirement |
| :--- | :--- |
| **Binaries** | Produce two binaries: `r-type_server` and `r-type_client`. |
| **Language** | Use C++. |
| **Build System** | Use **CMake** as the build system. |
| **Dependencies** | Use a **Package Manager** (Conan, Vcpkg, or CMake CPM) to handle 3rd-party dependencies. Project **MUST** be fully **self-contained**. |
| **Platform** | Project **MUST** run on **Linux** for both client and server. |
| **Cross-Platform** | Project **SHOULD** be **Cross-Platform**, running on Windows with Microsoft Visual C++ compiler. |
| **Workflow** | A well-defined software development workflow **MUST** be used (good Git practices like feature branches, merge requests, issues). |

### Documentation Requirements

| Category | Requirement |
| :--- | :--- |
| **Language** | **MUST** write the documentation in **English**. |
| **Core** | **MUST** include a **README** file. |
| **Core** | **MUST** produce documentation at a higher level than only class/function descriptions (Developer Documentation). |
| **Study** | Conduct a **Technical and Comparative Study** to justify technology choices (algorithms, storage, security). |
| **Access** | Make documentation available and accessible in a modern way (online/interlinked structured pages). |
| **Protocol** | Write **Protocol documentation** describing commands and packets, allowing someone to write a new client. |

### Accessibility Requirements

| Category | Requirement |
| :--- | :--- |
| **Disability** | Provide solutions for **Physical and Motor Disabilities**. |
| **Disability** | Provide solutions for **Audio and Visual Disabilities**. |
| **Disability** | Provide solutions for **Mental and Cognitive Disabilities**. |

### Core Architecture & Prototype

| Category | Requirement |
| :--- | :--- |
| **Core Goal** | Game **MUST** be **playable** with background, players, enemies, and missiles. |
| **Core Goal** | Game **MUST** be **networked**: distinct client connecting to a central authoritative server. |
| **Architecture** | Game **MUST** demonstrate engine foundations with decoupled subsystems/layers for **Rendering, Networking, and Game Logic**. |
| **Server** | Server **MUST** implement all game logic and act as the **authoritative source** of game events. |
| **Server** | Server **MUST** notify clients of all game events (spawns, moves, destroys, actions, and other clients' actions). |
| **Server** | Server **MUST** be **multithreaded**. |
| **Server** | Server **MUST** be robust; if a client crashes, the server **MUST** continue to work and notify others. |
| **Client** | Client **MUST** contain necessary elements for **display and player input** (server must have authority on final outcomes). |
| **Protocol** | **MUST** design a **binary protocol** for client/server communications. |
| **Protocol** | **MUST** use **UDP** for all in-game communications (entities, movements, events). |
| **Protocol** | Malformed messages **MUST NOT** lead to client or server crash, excessive memory consumption, or compromised security. |
| **Gameplay** | Client **MUST** display a slow horizontal scrolling background (star-field). |
| **Gameplay** | **MUST** use **timers**; overall time flow **MUST NOT** be tied to CPU speed. |
| **Gameplay** | Players **MUST** be able to move using the arrow keys. |
| **Gameplay** | **MUST** have Bydos slaves and missiles. Monsters **MUST** spawn randomly on the right. |
| **Gameplay** | The four players **MUST** be distinctly identifiable. |

### Track Advanced Architecture

| Category | Requirement |
| :--- | :--- |
| **Core Design** | Continue the design of your game engine to deliver a well-defined engine with proper abstractions, decoupling, features, and tools. |
| **Reusability** | Make the game engine a **generic and reusable building block**, delivered as a separate and standalone project. |
| **Reusability Proof** | Create a **2nd sample game** (different from R-Type) using your standalone game engine to prove its reusability The quality of the 2nd game (demo for engine features) will be evaluated.. |
| **Modularity** | Implement Modularity features (compile-time, link-time, or run-time plugin API). |
| **Features** | Implement essential Engine Subsystems (e.g., Rendering, Physics, Audio, HMI, Message Passing, Resource Management, Scripting). |
| **Tooling** | Implement Tooling features (e.g., Developer console with CVars, In-game metrics/profiling, World/scene/assets Editor). |