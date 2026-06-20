include(FetchContent)

# --- SFML ---
find_package(SFML 3.0 COMPONENTS Graphics Window System Audio Network CONFIG QUIET)
if (NOT SFML_FOUND)
    message(STATUS "SFML not found via vcpkg/system. Fetching from GitHub...")
    FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.0
    )
    FetchContent_MakeAvailable(sfml)
endif()

# --- Bullet3 ---
find_package(Bullet CONFIG QUIET)
if (NOT Bullet_FOUND)
    message(STATUS "Bullet not found. Fetching from GitHub...")
    FetchContent_Declare(
        bullet3
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG 3.25
        # Disable extras to speed up build
        CMAKE_ARGS -DBUILD_UNIT_TESTS=OFF -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF
    )
    FetchContent_MakeAvailable(bullet3)
endif()

# --- ZeroMQ (libzmq) ---
find_package(ZeroMQ CONFIG QUIET)
if (NOT ZeroMQ_FOUND)
    message(STATUS "ZeroMQ not found. Fetching from GitHub...")
    FetchContent_Declare(
        ZeroMQ
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG v4.3.5
        CMAKE_ARGS -DZMQ_BUILD_TESTS=OFF -DWITH_PERF_TOOL=OFF
    )
    FetchContent_MakeAvailable(ZeroMQ)
    
    # Aliasing for consistency with FindZeroMQ usually provided by vcpkg
    if (TARGET libzmq AND NOT TARGET libzmq-static)
        add_library(libzmq-static ALIAS libzmq)
    endif()
endif()

# --- cppzmq (ZeroMQ C++ Wrapper) ---
find_package(cppzmq CONFIG QUIET)
if (NOT cppzmq_FOUND)
    message(STATUS "cppzmq not found. Fetching from GitHub...")
    FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
        GIT_TAG v4.10.0
        CMAKE_ARGS -DCPPZMQ_BUILD_TESTS=OFF
    )
    FetchContent_MakeAvailable(cppzmq)
endif()

# --- Lua (Required for Sol2) ---
# Prefer normal discovery first (vcpkg toolchain or system packages).
find_package(Lua QUIET)

# If FindLua could not resolve it, try direct vcpkg-installed artifacts.
if (NOT LUA_FOUND AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_VCPKG_LUA_ROOT "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    if (EXISTS "${_VCPKG_LUA_ROOT}/include/lua.h")
        find_library(_VCPKG_LUA_LIB
            NAMES lua liblua
            PATHS "${_VCPKG_LUA_ROOT}/lib"
            NO_DEFAULT_PATH)
        if (_VCPKG_LUA_LIB)
            set(LUA_INCLUDE_DIR "${_VCPKG_LUA_ROOT}/include" CACHE PATH "Lua include dir" FORCE)
            set(LUA_LIBRARIES "${_VCPKG_LUA_LIB}" CACHE FILEPATH "Lua library" FORCE)
            set(LUA_FOUND TRUE CACHE BOOL "Lua found" FORCE)
            message(STATUS "Lua found via vcpkg_installed fallback: ${LUA_LIBRARIES}")
        endif()
    endif()
endif()

# Final fallback: build Lua from official source repository.
if (NOT LUA_FOUND)
    message(STATUS "Lua not found via vcpkg/system. Fetching from official Lua repository...")
    FetchContent_Declare(
        lua_src
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.6
    )
    FetchContent_GetProperties(lua_src)
    if (NOT lua_src_POPULATED)
        FetchContent_Populate(lua_src)

        add_library(lua STATIC
            ${lua_src_SOURCE_DIR}/lapi.c
            ${lua_src_SOURCE_DIR}/lauxlib.c
            ${lua_src_SOURCE_DIR}/lbaselib.c
            ${lua_src_SOURCE_DIR}/lcode.c
            ${lua_src_SOURCE_DIR}/lcorolib.c
            ${lua_src_SOURCE_DIR}/lctype.c
            ${lua_src_SOURCE_DIR}/ldblib.c
            ${lua_src_SOURCE_DIR}/ldebug.c
            ${lua_src_SOURCE_DIR}/ldo.c
            ${lua_src_SOURCE_DIR}/ldump.c
            ${lua_src_SOURCE_DIR}/lfunc.c
            ${lua_src_SOURCE_DIR}/lgc.c
            ${lua_src_SOURCE_DIR}/linit.c
            ${lua_src_SOURCE_DIR}/liolib.c
            ${lua_src_SOURCE_DIR}/llex.c
            ${lua_src_SOURCE_DIR}/lmathlib.c
            ${lua_src_SOURCE_DIR}/lmem.c
            ${lua_src_SOURCE_DIR}/loadlib.c
            ${lua_src_SOURCE_DIR}/lobject.c
            ${lua_src_SOURCE_DIR}/lopcodes.c
            ${lua_src_SOURCE_DIR}/loslib.c
            ${lua_src_SOURCE_DIR}/lparser.c
            ${lua_src_SOURCE_DIR}/lstate.c
            ${lua_src_SOURCE_DIR}/lstring.c
            ${lua_src_SOURCE_DIR}/lstrlib.c
            ${lua_src_SOURCE_DIR}/ltable.c
            ${lua_src_SOURCE_DIR}/ltablib.c
            ${lua_src_SOURCE_DIR}/ltm.c
            ${lua_src_SOURCE_DIR}/lundump.c
            ${lua_src_SOURCE_DIR}/lutf8lib.c
            ${lua_src_SOURCE_DIR}/lvm.c
            ${lua_src_SOURCE_DIR}/lzio.c
        )

        target_include_directories(lua PUBLIC ${lua_src_SOURCE_DIR})
        set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE ON)

        set(LUA_INCLUDE_DIR "${lua_src_SOURCE_DIR}" CACHE PATH "Lua include dir" FORCE)
        set(LUA_LIBRARIES lua CACHE STRING "Lua libraries" FORCE)
        set(LUA_FOUND TRUE CACHE BOOL "Lua found" FORCE)
    endif()
endif()

# --- Sol2 (Lua C++ Binding) ---
find_package(sol2 CONFIG QUIET)

# vcpkg's sol2 port is header-only and may not ship a CMake config file.
if (NOT sol2_FOUND)
    find_path(SOL2_INCLUDE_DIRS "sol/abort.hpp")
    if (SOL2_INCLUDE_DIRS)
        add_library(sol2 INTERFACE)
        target_include_directories(sol2 INTERFACE ${SOL2_INCLUDE_DIRS})
        if (NOT TARGET sol2::sol2)
            add_library(sol2::sol2 ALIAS sol2)
        endif()
        set(sol2_FOUND TRUE)
        message(STATUS "sol2 found via include path: ${SOL2_INCLUDE_DIRS}")
    endif()
endif()

if (NOT sol2_FOUND)
    message(STATUS "sol2 not found. Fetching from GitHub...")
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG v3.3.0
        CMAKE_ARGS -DSOL2_BUILD_LUA=OFF # We handle lua separately
    )
    FetchContent_MakeAvailable(sol2)
endif()

# --- Asio (Standalone) ---
find_package(asio CONFIG QUIET)
if (NOT asio_FOUND)
    message(STATUS "Asio not found. Fetching from GitHub...")
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-29-0
    )
    FetchContent_MakeAvailable(asio)
endif()

# --- msgpack-cxx ---
find_package(msgpack-cxx CONFIG QUIET)
if (NOT msgpack-cxx_FOUND)
    message(STATUS "msgpack-cxx not found. Fetching from GitHub...")
    FetchContent_Declare(
        msgpack-cxx
        GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
        GIT_TAG cpp-6.1.0
        CMAKE_ARGS -DMSGPACK_BUILD_TESTS=OFF
    )
    FetchContent_MakeAvailable(msgpack-cxx)
endif()
