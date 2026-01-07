# R-Type Game Executables

# Client Executable
add_executable(r-type_client
    client/main.cpp
    client/RTypeClient.cpp
    client/RTypeClient.hpp
    Rtype.cpp
    Rtype.hpp
    ../engine/app/AApplication.cpp
    ../engine/app/AApplication.hpp
    ../engine/app/IApplication.hpp
    ../engine/modulesManager/IModulesManager.hpp
    ../engine/modulesManager/AModulesManager.cpp
    ../engine/modulesManager/AModulesManager.hpp
    ../engine/modulesManager/ModulesManager.cpp
    ../engine/modulesManager/ModulesManager.hpp
)

# Server Executable
add_executable(r-type_server
    server/main.cpp
    server/RTypeServer.cpp
    server/RTypeServer.hpp
    Rtype.cpp
    Rtype.hpp
    ../engine/app/AApplication.cpp
    ../engine/app/AApplication.hpp
    ../engine/app/IApplication.hpp
    ../engine/modulesManager/IModulesManager.hpp
    ../engine/modulesManager/AModulesManager.cpp
    ../engine/modulesManager/AModulesManager.hpp
    ../engine/modulesManager/ModulesManager.cpp
    ../engine/modulesManager/ModulesManager.hpp
)

target_include_directories(r-type_client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
    ${CMAKE_SOURCE_DIR}/src/game
    ${CMAKE_SOURCE_DIR}/src/game/client
)

target_include_directories(r-type_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
    ${CMAKE_SOURCE_DIR}/src/game
    ${CMAKE_SOURCE_DIR}/src/game/server
)

# Find and link SFML Window component (Client only)
find_package(SFML COMPONENTS Window REQUIRED)
target_link_libraries(r-type_client PRIVATE SFML::Window)
# No SFML::Window for server

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

target_link_libraries(r-type_client PRIVATE
    cppzmq-static
)
target_link_libraries(r-type_server PRIVATE
    cppzmq-static
)

# Link with dynamic loading library
if(WIN32)
    # Windows uses built-in LoadLibrary
else()
    # Linux/Unix uses dl
    target_link_libraries(r-type_client PRIVATE ${CMAKE_DL_LIBS})
    target_link_libraries(r-type_server PRIVATE ${CMAKE_DL_LIBS})
endif()


# Set output names (already done by add_executable, but good practice for clarity)
set_target_properties(r-type_client PROPERTIES
    OUTPUT_NAME "r-type_client"
)
set_target_properties(r-type_server PROPERTIES
    OUTPUT_NAME "r-type_server"
)

# Copy modules to output directory after build
# Create a custom target to handle asset and library copying once to avoid race conditions
add_custom_target(prepare_game_env
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMENT "Copying assets to runtime directory"
)

# Collect all potential modules
set(ALL_MODULES 
    SFMLWindowManager 
    SFMLSoundManager 
    GLEWSFMLRenderer 
    LuaECSManager 
    ECSSavesManager 
    BulletPhysicEngine 
    NetworkManager
)

# Add module copy commands to the prepare target
foreach(MOD ${ALL_MODULES})
    if(TARGET ${MOD})
        add_dependencies(prepare_game_env ${MOD})
        add_custom_command(TARGET prepare_game_env POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${MOD}>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Copying ${MOD} to runtime directory"
        )
    endif()
endforeach()

# Bulk copy for non-Windows (legacy support or if other libs are present)
if(NOT WIN32)
    add_custom_command(TARGET prepare_game_env POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}/lib
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying all libs to runtime directory"
    )
endif()

# Make executables depend on the preparation target
foreach(EXECUTABLE_TARGET r-type_client r-type_server)
    add_dependencies(${EXECUTABLE_TARGET} prepare_game_env)
endforeach()

# Install rules
install(TARGETS r-type_client r-type_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)