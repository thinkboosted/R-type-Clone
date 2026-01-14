
find_package(GTest CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

add_executable(LuaECSManagerBindingTests
    LuaECSManagerBindingTests.cpp
    ../LuaECSManager.cpp
    ../LuaECSManager.hpp
    ../../../../core/Logger.cpp
    ../../../AModule.cpp
    ../../../IModule.hpp
)

target_link_libraries(LuaECSManagerBindingTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    cppzmq-static
    msgpack-cxx
    sol2::sol2
    asio
    ${LUA_LIBRARIES}
)

if(WIN32)
    target_link_libraries(LuaECSManagerBindingTests PRIVATE ws2_32 mswsock)
endif()

target_include_directories(LuaECSManagerBindingTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/modules/ECSManager/LuaECSManager
    ${CMAKE_SOURCE_DIR}/src/engine/core
)

# Register test with CTest
add_test(NAME LuaECSManagerBindingTests COMMAND LuaECSManagerBindingTests)
