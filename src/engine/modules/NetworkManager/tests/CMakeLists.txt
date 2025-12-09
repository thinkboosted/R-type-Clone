
find_package(GTest CONFIG REQUIRED)

add_executable(NetworkManagerTests
    NetworkManagerTests.cpp
    ../NetworkManager.cpp # We compile the source directly to access internals if needed, or link against lib
    ../NetworkManager.hpp
    ../../AModule.cpp
)

target_link_libraries(NetworkManagerTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    asio
    msgpack-cxx
    cppzmq
    libzmq
)

if(WIN32)
    target_link_libraries(NetworkManagerTests PRIVATE ws2_32 mswsock)
endif()

target_include_directories(NetworkManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules/NetworkManager
)

add_test(NAME NetworkManagerTests COMMAND NetworkManagerTests)
