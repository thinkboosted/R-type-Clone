# R-Type Game Executable
add_executable(rtype_game
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

target_include_directories(rtype_game PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
    ${CMAKE_SOURCE_DIR}/src/game
)

# Link with ZeroMQ for message broker
target_link_libraries(rtype_game
    cppzmq
    libzmq
)

# Link with dynamic loading library
if(WIN32)
    # Windows uses built-in LoadLibrary
else()
    # Linux/Unix uses dl
    target_link_libraries(rtype_game ${CMAKE_DL_LIBS})
endif()

# Set output name
set_target_properties(rtype_game PROPERTIES
    OUTPUT_NAME "rtype"
)

# Copy modules to output directory after build (only if they exist)
if(TARGET SFMLWindowManager)
    add_custom_command(TARGET rtype_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFMLWindowManager>
            $<TARGET_FILE_DIR:rtype_game>
        COMMENT "Copying SFML Window Manager to executable directory"
    )
endif()

if(TARGET GLEWSFMLRenderer)
    add_custom_command(TARGET rtype_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GLEWSFMLRenderer>
            $<TARGET_FILE_DIR:rtype_game>
        COMMENT "Copying GLEWSFML Renderer to executable directory"
    )
endif()

if(TARGET EngineSceneManager)
    add_custom_command(TARGET rtype_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:EngineSceneManager>
            $<TARGET_FILE_DIR:rtype_game>
        COMMENT "Copying Engine Scene Manager to executable directory"
    )
endif()

if(TARGET LuaECS)
    add_custom_command(TARGET rtype_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LuaECS>
            $<TARGET_FILE_DIR:rtype_game>
        COMMENT "Copying LuaECS to executable directory"
    )
    
    if(WIN32)
        # Find and copy liblua.dll
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            find_file(LUA_DLL liblua.dll PATHS ${CMAKE_PREFIX_PATH}/debug/bin NO_DEFAULT_PATH)
        else()
            find_file(LUA_DLL liblua.dll PATHS ${CMAKE_PREFIX_PATH}/bin NO_DEFAULT_PATH)
        endif()

        if(LUA_DLL)
            add_custom_command(TARGET rtype_game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${LUA_DLL}"
                    $<TARGET_FILE_DIR:rtype_game>
                COMMENT "Copying liblua.dll to executable directory"
            )
        else()
            message(WARNING "Could not find liblua.dll to copy")
        endif()
    endif()
endif()

if(TARGET BulletPhysicEngine)
    add_custom_command(TARGET rtype_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:BulletPhysicEngine>
            $<TARGET_FILE_DIR:rtype_game>
        COMMENT "Copying BulletPhysicEngine to executable directory"
    )
endif()

# Install rules
install(TARGETS rtype_game
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
