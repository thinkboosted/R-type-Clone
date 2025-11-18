# Build instructions

## Native build

```bash
mkdir -p build_native
cd build_native
cmake ..
cmake --build . -j4
./opengl_app
```

## Web build (Emscripten)

```bash
source /home/$USER/emsdk/emsdk_env.sh
mkdir -p build_web
cd build_web
emcmake cmake ..
cmake --build . -j
python3 -m http.server 8080
```

Open `http://localhost:8080/opengl_app.html` to see the WebAssembly build.

## Third-party libraries (`libs/`)

The `libs` directory stores vendored dependencies that CMake pulls directly into the build. Currently it contains `glm-0.9.9.8`, a header-only math library used for matrix/vector operations in the renderer. Keeping glm locally:

- ensures consistent builds across native and web targets without requiring a system-wide install;
- allows CMake to add the library via `add_subdirectory(libs/glm-0.9.9.8)`;
- keeps the original documentation (manual, API reference) available offline.

If new third-party libraries are needed in the future, place them under `libs/` and update `CMakeLists.txt` to include them so other developers can build immediately.
