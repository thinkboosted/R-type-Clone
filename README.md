# R-type-Clone

Minimal instructions to build and run the R-type-Clone project on Linux and Windows.

## Prerequisites

- Git
- CMake (>= 3.15 recommended)
- A C++ toolchain (GCC/Clang on Linux, MSVC or MSVC+vcpkg on Windows)
- vcpkg (recommended) — project includes `vcpkg.json` to describe dependencies

Notes
- This project uses CMake as its build system. A `CMakePresets.json` is present and can be used on platforms that support CMake presets.
- We recommend using vcpkg (manifest mode) to install and manage native dependencies consistently across platforms.

## Quick build — Linux (recommended using vcpkg)

1. Clone the repository:

```bash
git clone https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone
```

2. Bootstrap vcpkg (first time only):

```bash
./vcpkg/bootstrap-vcpkg.sh
```

3. Install dependencies from the manifest:

```bash
./vcpkg/vcpkg install --manifest
```

4. Configure and build with CMake:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc)
```

Notes:
- If you prefer system packages (Ubuntu/Debian), you can install development packages directly (example):

```bash
sudo apt update
sudo apt install build-essential cmake git libsfml-dev libglew-dev libbullet-dev
```

Then configure CMake without the vcpkg toolchain file:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc)
```

## Quick build — Windows (PowerShell, recommended using vcpkg)

1. Clone the repository (PowerShell):

```powershell
git clone https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone
```

2. Bootstrap vcpkg (first time only):

```powershell
.\vcpkg\bootstrap-vcpkg.bat
```

3. Install dependencies from the manifest (x64 example):

```powershell
.\vcpkg\vcpkg.exe install --triplet x64-windows --manifest
```

4. Configure and build with CMake (MSVC/x64 example):

```powershell
mkdir build; cd build
cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Using CMake presets

This project contains a `CMakePresets.json`. To list available presets run:

```bash
cmake --list-presets
```

Then configure using a preset (if a preset exists):

```bash
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

## Run

After a successful build, the executable produced by the project will be in the build folder (or a subfolder such as `bin/` depending on the CMake configuration).

Run the produced binary (example):

```bash
# from build/
./Rtype    # or the actual produced executable name
```

If you have trouble finding the executable, inspect the CMake or build output for the target name.

## Troubleshooting

- If CMake cannot find dependencies, ensure you used the vcpkg toolchain file or installed system packages.
- On Windows prefer MSVC (Visual Studio) for easiest compatibility with vcpkg. On Linux prefer GCC or Clang.
- If rendering or linking errors occur, check which backend is enabled in the repository's modules (SFML, GLEW, Bullet are used by modules in `src/modules/`).

## Where to go next

- See `docs/` for project documentation.
- See `CONTRIBUTING.md` for how to contribute and the required commit message format (Conventional Commits).
