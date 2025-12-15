# R-Type Game Executables
add_executable(r-type_client
    main.cpp
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

add_executable(r-type_server
    main.cpp
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
)

target_include_directories(r-type_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
    ${CMAKE_SOURCE_DIR}/src/game
)

# Find and link SFML Window component
find_package(SFML COMPONENTS Window REQUIRED)
target_link_libraries(r-type_client PRIVATE SFML::Window)
target_link_libraries(r-type_server PRIVATE SFML::Window)

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

# Copy modules to output directory after build (only if they exist)
# Apply for both client and server executables
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

    if(TARGET EngineSceneManager)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:EngineSceneManager>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying Engine Scene Manager to executable directory for ${EXECUTABLE_TARGET}"
        )
    endif()

    if(TARGET LuaECS)
        add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:LuaECS>
                $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
            COMMENT "Copying LuaECS to executable directory for ${EXECUTABLE_TARGET}"
        )

        if(WIN32)
            # Find and copy liblua.dll
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                find_file(LUA_DLL liblua.dll PATHS ${CMAKE_PREFIX_PATH}/debug/bin NO_DEFAULT_PATH)
            else()
                find_file(LUA_DLL liblua.dll PATHS ${CMAKE_PREFIX_PATH}/bin NO_DEFAULT_PATH)
            endif()

            if(LUA_DLL)
                add_custom_command(TARGET ${EXECUTABLE_TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${LUA_DLL}"
                        $<TARGET_FILE_DIR:${EXECUTABLE_TARGET}>
                    COMMENT "Copying liblua.dll to executable directory for ${EXECUTABLE_TARGET}"
                )
            else()
                message(WARNING "Could not find liblua.dll to copy for ${EXECUTABLE_TARGET}")
            endif()
        endif()
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
endforeach()

# Install rules
install(TARGETS r-type_client r-type_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)