# 3D Platformer Game Executable

add_executable(platformer
    main.cpp
    PlatformerGame.cpp
    PlatformerGame.hpp
    ../engine/app/AApplication.cpp
    ../engine/app/AApplication.hpp
    ../engine/app/IApplication.hpp
    ../engine/modulesManager/IModulesManager.hpp
    ../engine/modulesManager/AModulesManager.cpp
    ../engine/modulesManager/AModulesManager.hpp
    ../engine/modulesManager/ModulesManager.cpp
    ../engine/modulesManager/ModulesManager.hpp
)

target_include_directories(platformer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
    ${CMAKE_SOURCE_DIR}/src/game
    ${CMAKE_SOURCE_DIR}/src/platformer
)

# Find and link SFML Window component
find_package(SFML COMPONENTS Window REQUIRED)
target_link_libraries(platformer PRIVATE SFML::Window)

# Link with ZeroMQ for message broker
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

target_link_libraries(platformer PRIVATE
    cppzmq
)

# Link with dynamic loading library
if(WIN32)
    # Windows uses built-in LoadLibrary
else()
    # Linux/Unix uses dl
    target_link_libraries(platformer PRIVATE ${CMAKE_DL_LIBS})
endif()

# Set output name
set_target_properties(platformer PROPERTIES
    OUTPUT_NAME "platformer"
)

# Add dependency on prepare_game_env to ensure assets and modules are copied
add_dependencies(platformer prepare_game_env)

# Install rules
install(TARGETS platformer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
