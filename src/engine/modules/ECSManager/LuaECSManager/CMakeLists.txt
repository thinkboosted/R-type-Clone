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

# Try find sol2 via CONFIG first, then pkg-config, then header search
find_package(sol2 CONFIG QUIET)
if(NOT sol2_FOUND)
    # Try pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SOL2 QUIET sol2)
    endif()
    
    if(NOT SOL2_FOUND)
        # Fallback: search for headers directly
        find_path(SOL2_INCLUDE_DIR sol/sol.hpp
            PATHS /usr/include /usr/local/include /opt/include
            PATH_SUFFIXES sol2
        )
        if(SOL2_INCLUDE_DIR)
            message(STATUS "Found sol2 headers at ${SOL2_INCLUDE_DIR}")
        else()
            message(FATAL_ERROR "sol2 not found. Please install libsol2-dev or provide sol2 via vcpkg")
        endif()
    endif()
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
    ${SOL2_INCLUDE_DIRS}
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
