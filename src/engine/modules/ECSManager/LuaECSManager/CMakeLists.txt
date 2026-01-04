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

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

# Ensure sol2 gets Lua includes
target_include_directories(LuaECSManager PRIVATE ${LUA_INCLUDE_DIR})

target_link_libraries(LuaECSManager
    cppzmq
    libzmq
    ${LUA_LIBRARIES}
    sol2::sol2
)

set_target_properties(LuaECSManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "LuaECSManager"
)

install(TARGETS LuaECSManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
