# Cross-Platform Development Guide: Windows vs. Linux

This document outlines the key differences between Windows and Linux development contexts for our Game Engine. Understanding these differences is crucial for maintaining a stable build on both platforms and debugging OS-specific issues.

## 1. Dynamic Library Exports (DLL vs. SO)

This is the most common cause of compilation errors when adding new modules.

### The Difference
*   **Linux (`.so`):** By default, all symbols (functions, classes) in a shared library are **visible** and accessible.
*   **Windows (`.dll`):** By default, all symbols are **hidden**. You must explicitly tell the compiler which functions to "export" so the main application can use them.

### The Solution: Export Macros
We use preprocessor macros to handle this. Never write `__declspec(dllexport)` directly in your code.

**Pattern to follow:**
```cpp
#ifdef _WIN32
    #define MY_MODULE_EXPORT __declspec(dllexport)
#else
    #define MY_MODULE_EXPORT
#endif

// Usage
extern "C" MY_MODULE_EXPORT IModule* createModule(...) { ... }
```

## 2. Build System & Linking (CMake)

CMake handles most cross-platform tasks, but the linker behaves differently.

### Windows
*   **Libraries:** Uses `.lib` for linking and `.dll` for runtime.
*   **Runtime:** Executables need the `.dll` files in the same directory (or in `%PATH%`).
    *   *Action:* We use `add_custom_command` in CMake to copy `.dll` files to the build folder.
*   **System Libs:** Standard libraries (like `ws2_32` for sockets) are often linked automatically or via standard flags.

### Linux
*   **Libraries:** Uses `.a` (static) or `.so` (shared).
*   **Runtime:** Uses `RPATH` or `LD_LIBRARY_PATH` to find shared objects.
*   **System Libs:** You must explicitly link low-level libraries that are implicitly included on Windows.
    *   **X11:** Required for window management (`target_link_libraries(Target X11::X11)`).
    *   **dl:** Required for dynamic loading (`CMAKE_DL_LIBS`).
    *   **pthread:** Required for threading.

## 3. Windowing & OpenGL Context

To render 3D graphics, we need to attach OpenGL to a native OS window. This implementation is completely different between OSs.

*   **Windows:** Uses **WGL** (Windows-GL).
    *   Handles: `HWND` (Window) and `HDC` (Device Context).
    *   Headers: `windows.h`.
*   **Linux:** Uses **GLX** (OpenGL extension to the X Window System).
    *   Handles: `Display*` (Connection to X server) and `Window` (XID).
    *   Headers: `X11/Xlib.h`, `GL/glx.h`.

**Best Practice:** Keep OS-specific code isolated in the `.cpp` files of the Renderer module (e.g., `GLEWRenderer.cpp`). Never expose `windows.h` or `Xlib.h` in public header files, as this pollutes the global namespace.

## 4. Module Loading (Dynamic Loading)

The C++ code to load our game modules differs by platform.

*   **Windows:**
    *   Function: `LoadLibraryA` / `GetProcAddress`.
    *   File Extension: `.dll`.
*   **Linux:**
    *   Function: `dlopen` / `dlsym`.
    *   File Extension: `.so`.

**Note for Gameplay Programmers:** When configuring modules in `Rtype.cpp`, remember to handle the file extension:

```cpp
#ifdef _WIN32
    loadModule("MySystem.dll");
#else
    loadModule("MySystem.so");
#endif
```

## 5. Assets & File Paths

### Path Separators
*   **Windows:** Traditionally `\`, but supports `/`.
*   **Linux:** Strictly `/`.
*   **Rule:** **ALWAYS use forward slashes (`/`)** in code and CMake. It works on both.
    *   *Good:* `"assets/models/cube.obj"`
    *   *Bad:* `"assets\\models\\cube.obj"`

### Case Sensitivity (Critical!)
*   **Windows:** Case-insensitive. `Texture.png` and `texture.png` are the same file.
*   **Linux:** **Case-sensitive.** `Texture.png` and `texture.png` are different files.
*   **The Trap:** If you reference `sprite.png` in your code but the file is named `Sprite.png`, it will work on your Windows machine but **crash on the Linux CI/CD**.
*   **Rule:** Stick to a naming convention (e.g., snake_case) for all assets and match it exactly in code.

## Summary Checklist for PRs

Before submitting a Pull Request:
1. [ ] Did you use `/` for all file paths?
2. [ ] Is the case of your filenames matching your code exactly?
3. [ ] If you added a new module, did you add the Export Macro?
4. [ ] If you used a new library, did you check if Linux needs an extra `target_link_libraries` in CMake (like X11 or dl)?
