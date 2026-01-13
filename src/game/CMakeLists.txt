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
# The specific modules copied are now determined by what each executable loads in main.cpp.
# So, we just need to ensure ALL potential modules are considered for copying if they exist.
# The 'addModule' calls in C++ will find them.
foreach(EXECUTABLE_TARGET r-type_client r-type_server)
    if(TARGET SFMLWindowManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SFMLWindowManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying SFML Window Manager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET SFMLSoundManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SFMLSoundManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying SFML Sound Manager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET GLEWSFMLRenderer)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:GLEWSFMLRenderer>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying GLEWSFML Renderer to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET LuaECSManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:LuaECSManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying LuaECSManager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET ECSSavesManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:ECSSavesManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying ECSSavesManager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET BulletPhysicEngine)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:BulletPhysicEngine>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying BulletPhysicEngine to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET NetworkManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:NetworkManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying NetworkManager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    # Ensure modules are built before the executable (so their artifacts exist for copying)
    if(TARGET SFMLWindowManager)
        add_dependencies(${EXECUTABLE_TARGET} SFMLWindowManager)
    endif()
    if(TARGET GLEWSFMLRenderer)
        add_dependencies(${EXECUTABLE_TARGET} GLEWSFMLRenderer)
    endif()
    if(TARGET LuaECSManager)
        add_dependencies(${EXECUTABLE_TARGET} LuaECSManager)
    endif()
    if(TARGET ECSSavesManager)
        add_dependencies(${EXECUTABLE_TARGET} ECSSavesManager)
    endif()
    if(TARGET BulletPhysicEngine)
        add_dependencies(${EXECUTABLE_TARGET} BulletPhysicEngine)
    endif()
    if(TARGET NetworkManager)
        add_dependencies(${EXECUTABLE_TARGET} NetworkManager)
    endif()

    # Copy all .so modules from lib/ directory (Linux/Unix only)
    if(NOT WIN32)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_BINARY_DIR}/lib
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying all module libraries to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

endforeach()

# Create a shared target for copying assets to avoid race conditions during parallel builds
add_custom_target(CopyAssets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
    VERBATIM
)
add_dependencies(r-type_client CopyAssets)
add_dependencies(r-type_server CopyAssets)

# Install rules
install(TARGETS r-type_client r-type_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)