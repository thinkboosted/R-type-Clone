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
find_package(Lua QUIET)
if (NOT LUA_FOUND)
    message(STATUS "Lua not found. Fetching from GitHub (lua-cmake mirror)...")
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/waltersugar/lua.git
        GIT_TAG master 
    )
    FetchContent_MakeAvailable(lua)
    # Create the 'lua' alias target that FindLua usually provides or expects linked
    if (TARGET liblua AND NOT TARGET lua)
        add_library(lua ALIAS liblua)
    endif()
endif()

# --- Sol2 (Lua C++ Binding) ---
find_package(sol2 CONFIG QUIET)
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
