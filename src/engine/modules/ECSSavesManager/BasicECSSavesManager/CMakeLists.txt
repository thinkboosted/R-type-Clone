add_library(ECSSavesManager SHARED
    BasicECSSavesManager.cpp
    BasicECSSavesManager.hpp
    ../IECSSavesManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(ECSSavesManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

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
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include
    )
endif()

target_link_libraries(ECSSavesManager
    ${ZeroMQ_LINK_LIBS}
)

target_include_directories(ECSSavesManager PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
)

set_target_properties(ECSSavesManager PROPERTIES PREFIX "")
