# Build & run guide

This project supports a native OpenGL build (desktop) and a WebAssembly build via Emscripten. All third-party libraries (GLFW + glm) are downloaded automatically with CMake’s `FetchContent`, so the repository stays light.

## 1. Prerequisites

### Native desktop

- CMake ≥ 3.10
- A C++17 compiler (GCC 9+, Clang 12+, MSVC 2019+)
- OpenGL drivers
- GLEW + GLUT development packages (on Debian/Ubuntu: `sudo apt install build-essential cmake libglew-dev freeglut3-dev`)

### Web (Emscripten)

- [emsdk](https://emscripten.org/docs/getting_started/downloads.html) installed locally
- Run `source /path/to/emsdk/emsdk_env.sh` before configuring/building so `emcmake`/`emcc` are on your PATH

## 2. Native build & run

```bash
mkdir -p build_native
cd build_native
cmake ..
cmake --build . -j4
./opengl_app
```

The executable opens a GLUT window with the spinning hexagon demo.

## 3. WebAssembly build & run

```bash
source /home/$USER/emsdk/emsdk_env.sh
mkdir -p build_web
cd build_web
emcmake cmake ..
cmake --build . -j
python3 -m http.server 8080
```

Then open `http://localhost:8080/opengl_app.html` in a browser. Emscripten emits `opengl_app.html/.js/.wasm` inside `build_web/`.

## 4. Dependency notes

- **GLFW**: downloaded at configure time for both native and web builds. No system install required.
- **glm**: also fetched automatically; the old vendored copy under `libs/` is no longer needed. If you need an offline build, download the glm archive once and point `FetchContent` to a local mirror (see comments in `CMakeLists.txt`).
- **GLEW / GLUT**: still provided by the host OS for the native build, so install the dev packages listed above.

## 5. Tips

- Re-run `cmake ..` whenever you switch between native and web build folders so the correct toolchain is configured.
- If CMake cannot reach GitHub (offline builds), pre-download the glm & GLFW archives and adjust the `GIT_REPOSITORY` URLs accordingly.
- To clean a build, remove the corresponding `build_*` directory and re-run the steps.
