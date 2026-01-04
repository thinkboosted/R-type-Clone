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
)

# Find Lua first and immediately propagate includes/libs
find_package(Lua REQUIRED)
target_include_directories(LuaECSManager PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(LuaECSManager ${LUA_LIBRARIES})

# Find sol2 (vcpkg) then fallback to FetchContent
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
    target_include_directories(LuaECSManager PRIVATE ${sol2_SOURCE_DIR}/include)
else()
    target_link_libraries(LuaECSManager sol2::sol2)
endif()

# Find ZeroMQ (vcpkg) then fallback to pkg-config
find_package(ZeroMQ CONFIG QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZeroMQ REQUIRED libzmq)
    target_include_directories(LuaECSManager PRIVATE ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(LuaECSManager ${ZeroMQ_LIBRARIES})
else()
    target_link_libraries(LuaECSManager libzmq)
endif()

# Find cppzmq (header-only)
find_package(cppzmq CONFIG QUIET)
if(NOT cppzmq_FOUND)
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include /opt/include
    )
    if(CPPZMQ_INCLUDE_DIR)
        target_include_directories(LuaECSManager PRIVATE ${CPPZMQ_INCLUDE_DIR})
    endif()
else()
    target_link_libraries(LuaECSManager cppzmq)
endif()

set_target_properties(LuaECSManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "LuaECSManager"
)

install(TARGETS LuaECSManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
