add_library(LuaECSManager SHARED
    LuaECSManager.cpp
    LuaBindings.cpp
    LuaSerialization.cpp
    MsgPackUtils.cpp
    MsgPackUtils.hpp
    LuaECSManager.hpp
    ../IECSManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(LuaECSManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)

target_link_libraries(LuaECSManager
    cppzmq
    ${LUA_LIBRARIES}
    ${ZeroMQ_LINK_LIBS}
    msgpack-cxx
)

if(TARGET sol2::sol2)
    target_link_libraries(LuaECSManager sol2::sol2)
endif()

target_include_directories(LuaECSManager PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
)

set_target_properties(LuaECSManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "LuaECSManager"
)

install(TARGETS LuaECSManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
