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
target_link_libraries(r-type_client PRIVATE
    cppzmq
    libzmq
)
target_link_libraries(r-type_server PRIVATE
    cppzmq
    libzmq
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

    add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>/assets
        COMMENT "Copying assets to executable directory for ${EXECUTABLE_TARGET}"
    )
endforeach()

# Install rules
install(TARGETS r-type_client r-type_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)