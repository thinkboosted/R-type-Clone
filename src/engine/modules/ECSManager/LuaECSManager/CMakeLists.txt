add_library(LuaECSManager SHARED
    LuaECSManager.cpp
    LuaECSManager.hpp
    ../IECSManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(LuaECSManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${LUA_INCLUDE_DIR}
)

find_package(Lua 5.4 REQUIRED)

# Try to find sol2 via CONFIG (vcpkg), then fallback to FetchContent
find_package(sol2 CONFIG QUIET)
if(NOT sol2_FOUND)
    message(STATUS "sol2 not found via find_package, using FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG v3.3.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sol2)
    set(SOL2_INCLUDE_DIR ${sol2_SOURCE_DIR}/include)
endif()

# Try find_package with CONFIG first (for vcpkg), fall back to pkg-config
find_package(ZeroMQ CONFIG QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZeroMQ REQUIRED libzmq)
endif()

find_package(cppzmq CONFIG QUIET)
if(NOT cppzmq_FOUND)
    # cppzmq is header-only
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include /opt/include
    )
endif()

# Ensure sol2 gets Lua includes
target_include_directories(LuaECSManager PRIVATE 
    ${LUA_INCLUDE_DIR}
    ${SOL2_INCLUDE_DIR}
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
)

target_link_libraries(LuaECSManager
    ${ZeroMQ_LIBRARIES}
    ${LUA_LIBRARIES}
)

set_target_properties(LuaECSManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "LuaECSManager"
)

install(TARGETS LuaECSManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
