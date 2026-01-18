# ⚡ Quick Start

This page has been consolidated into the main Installation Guide.

**Please see: [📦 Installation Guide](INSTALL.md)**

---

## Quick Reference

### Build & Run (Linux)

```bash
git clone --recursive https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone
python3 build.py
cd build
./r-type_client local
```

### Build & Run (Windows)

```powershell
git clone --recursive https://github.com/thinkboosted/R-type-Clone.git
cd R-type-Clone
.\vcpkg\bootstrap-vcpkg.bat
mkdir build; cd build
cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
.\Release\r-type_client.exe local
```

For detailed instructions, see [INSTALL.md](INSTALL.md).

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
