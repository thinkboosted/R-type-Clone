# 📦 Installation Guide

This guide provides detailed instructions for building and running the R-Type Clone project on Linux and Windows.

---

## 📋 Table of Contents

- [Prerequisites](#prerequisites)
- [Linux Installation](#-linux-installation)
- [Windows Installation](#-windows-installation)
- [CMake Presets](#-cmake-presets)
- [Troubleshooting](#-troubleshooting)

---

## Prerequisites

### Required Tools

| Tool | Minimum Version | Purpose |
|------|-----------------|---------|
| Git | 2.x | Version control |
| CMake | 3.15+ | Build system |
| Python | 3.x | Build scripts |
| C++ Compiler | C++17 support | Compilation |

### Supported Compilers

- **Linux**: GCC 9+, Clang 10+
- **Windows**: MSVC 2019+, Clang-CL

---

## 🐧 Linux Installation

### Option 1: Using vcpkg (Recommended)

vcpkg handles all dependencies automatically.

```bash
# 1. Clone the repository with submodules
git clone --recursive https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone

# 2. Bootstrap vcpkg (first time only)
./vcpkg/bootstrap-vcpkg.sh

# 3. Build using the helper script
python3 build.py

# 4. Run the game
cd build
export LD_LIBRARY_PATH=$PWD:$PWD/lib:$LD_LIBRARY_PATH
./r-type_client local
```

### Option 2: Manual CMake Build

```bash
# 1. Clone the repository
git clone --recursive https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone

# 2. Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.sh

# 3. Create build directory
mkdir -p build && cd build

# 4. Configure with CMake
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# 5. Build
cmake --build . --config Release -j$(nproc)

# 6. Run
export LD_LIBRARY_PATH=$PWD:$PWD/lib:$LD_LIBRARY_PATH
./r-type_client local
```

### Option 3: System Packages (Ubuntu/Debian)

If you prefer system packages over vcpkg:

```bash
# Install dependencies
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libsfml-dev \
    libglew-dev \
    libbullet-dev \
    libzmq3-dev \
    liblua5.4-dev \
    libasio-dev \
    libmsgpack-dev \
    libgtest-dev

# Clone and build
git clone https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc)
```

---

## 🪟 Windows Installation

### Using vcpkg with Visual Studio

```powershell
# 1. Clone the repository
git clone --recursive https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone

# 2. Bootstrap vcpkg (first time only)
.\vcpkg\bootstrap-vcpkg.bat

# 3. Create build directory
mkdir build
cd build

# 4. Configure with CMake (x64)
cmake .. -A x64 `
    -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake `
    -DCMAKE_BUILD_TYPE=Release

# 5. Build
cmake --build . --config Release

# 6. Run
.\Release\r-type_client.exe local
```

### Using Visual Studio IDE

1. Open Visual Studio 2019 or later
2. Select **File** → **Open** → **CMake...**
3. Navigate to the cloned repository and open `CMakeLists.txt`
4. Visual Studio will automatically configure the project
5. Select the build configuration (Release recommended)
6. Build with **Build** → **Build All**

---

## ⚙️ CMake Presets

The project includes `CMakePresets.json` for convenient configuration:

```bash
# List available presets
cmake --list-presets

# Configure using a preset
cmake --preset=linux-release
cmake --build --preset=linux-release
```

### Available Presets

| Preset | Platform | Description |
|--------|----------|-------------|
| `linux-debug` | Linux | Debug build with symbols |
| `linux-release` | Linux | Optimized release build |
| `windows-debug` | Windows | Debug build with symbols |
| `windows-release` | Windows | Optimized release build |

---

## 🎮 Running the Game

### Solo Mode

```bash
./r-type_client local
```

### Multiplayer Mode

**Start the server:**
```bash
./r-type_server <port>
# Example:
./r-type_server 4242
```

**Connect as client:**
```bash
./r-type_client <server_ip> <port>
# Example:
./r-type_client 127.0.0.1 4242
```

### Sound Test

```bash
./sound_test
```

---

## 🔧 Troubleshooting

### Linux: Library Not Found

If you get library loading errors:

```bash
# Set library path before running
export LD_LIBRARY_PATH=$PWD:$PWD/lib:$LD_LIBRARY_PATH
./r-type_client local
```

### vcpkg: Lock File Error

If vcpkg fails with a lock error:

```bash
# Remove stale lock files
rm -f vcpkg/vcpkg-lock.json
pkill -f vcpkg  # Kill any stale processes
```

### CMake: Cannot Find Package

Ensure vcpkg toolchain is specified:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Windows: DLL Not Found

Copy the required DLLs to the executable directory or add the vcpkg bin folder to PATH:

```powershell
$env:PATH += ";$PWD\..\vcpkg\installed\x64-windows\bin"
```

### Build Fails with Missing Headers

Ensure submodules are initialized:

```bash
git submodule update --init --recursive
```

---

## 📊 Build Outputs

After a successful build, you'll find:

| File | Description |
|------|-------------|
| `r-type_client` | Game client executable |
| `r-type_server` | Game server executable |
| `sound_test` | Audio system test utility |
| `*.so` / `*.dll` | Dynamic module libraries |

---

## 🔗 See Also

- [Architecture Overview](ARCHITECTURE.md)
- [Contributing Guide](CONTRIBUTING.md)
- [Network Protocol](NETWORK_PROTOCOL.md)
