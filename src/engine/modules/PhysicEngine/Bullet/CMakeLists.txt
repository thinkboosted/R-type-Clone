# Bullet Physics Engine Module
add_library(BulletPhysicEngine SHARED
    BulletPhysicEngine.cpp
    BulletPhysicEngine.hpp
    ../IPhysicEngine.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(BulletPhysicEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

find_package(Bullet CONFIG REQUIRED)

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

target_link_libraries(BulletPhysicEngine
    BulletDynamics
    BulletCollision
    LinearMath
    ${ZeroMQ_LINK_LIBS}
)

target_include_directories(BulletPhysicEngine PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
)

# Set output name without lib prefix
set_target_properties(BulletPhysicEngine PROPERTIES
    PREFIX ""
    OUTPUT_NAME "BulletPhysicEngine"
)

# Install rules
install(TARGETS BulletPhysicEngine
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
