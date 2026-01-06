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
find_package(cppzmq CONFIG REQUIRED)

target_link_libraries(BulletPhysicEngine
    BulletDynamics
    BulletCollision
    LinearMath
    cppzmq-static
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
