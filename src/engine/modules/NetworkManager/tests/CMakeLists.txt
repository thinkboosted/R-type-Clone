
find_package(GTest CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

add_executable(NetworkManagerTests
    NetworkManagerTests.cpp
    ../NetworkManager.cpp # Compiling source directly; tests use only the public interface (black-box testing)
    ../NetworkManager.hpp
    ../../AModule.cpp
    ../../../core/Logger.cpp
)

target_link_libraries(NetworkManagerTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    asio
    msgpack-cxx
    cppzmq-static
)

if(WIN32)
    target_link_libraries(NetworkManagerTests PRIVATE ws2_32 mswsock)
endif()

target_include_directories(NetworkManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules/NetworkManager
)

add_test(NAME NetworkManagerTests COMMAND NetworkManagerTests)
