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
find_package(sol2 CONFIG QUIET)
if(NOT sol2_FOUND)
    # Try fallback method for sol2
    find_path(SOL2_INCLUDE_DIR sol.hpp PATHS /usr/include /usr/local/include)
    if(NOT SOL2_INCLUDE_DIR)
        message(FATAL_ERROR "sol2 not found and fallback search failed")
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
