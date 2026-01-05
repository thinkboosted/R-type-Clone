add_library(NetworkManager SHARED
    NetworkManager.cpp
    NetworkManager.hpp
    INetworkManager.hpp
    ../IModule.hpp
    ../AModule.hpp
    ../AModule.cpp
)

target_include_directories(NetworkManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

find_package(asio CONFIG REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)

# Try find_package with CONFIG first (for vcpkg), fall back to pkg-config
find_package(ZeroMQ CONFIG QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZeroMQ REQUIRED libzmq)
    set(ZeroMQ_LINK_LIBS ${ZeroMQ_LINK_LIBRARIES})
else()
    set(ZeroMQ_LINK_LIBS libzmq)
endif()

find_package(cppzmq CONFIG QUIET)
if(NOT cppzmq_FOUND)
    # cppzmq is header-only
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include
    )
endif()

set(ASIO_TARGET "")
if(TARGET asio::asio)
    set(ASIO_TARGET asio::asio)
elseif(TARGET asio)
    set(ASIO_TARGET asio)
endif()

set(MSGPACK_TARGET "")
if(TARGET msgpackc-cxx)
    set(MSGPACK_TARGET msgpackc-cxx)
elseif(TARGET msgpackc-cxx-static)
    set(MSGPACK_TARGET msgpackc-cxx-static)
elseif(TARGET msgpackc-cxx-shared)
    set(MSGPACK_TARGET msgpackc-cxx-shared)
elseif(TARGET msgpack-cxx)
    set(MSGPACK_TARGET msgpack-cxx)
endif()

if(ASIO_TARGET)
    target_link_libraries(NetworkManager PRIVATE ${ASIO_TARGET})
endif()

if(MSGPACK_TARGET)
    target_link_libraries(NetworkManager PRIVATE ${MSGPACK_TARGET})
endif()

target_link_libraries(NetworkManager PRIVATE
    ${ZeroMQ_LINK_LIBS}
)

target_include_directories(NetworkManager PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
)

if(WIN32)
    target_link_libraries(NetworkManager PRIVATE ws2_32 mswsock)
endif()

set_target_properties(NetworkManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "NetworkManager"
)

install(TARGETS NetworkManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

enable_testing()
add_subdirectory(tests)
