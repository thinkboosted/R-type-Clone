# Sound Test Executable
#
# This creates a simple test executable that only loads the SFMLSoundManager
# to verify that the sound system works correctly.

add_executable(sound_test
    main.cpp
    ../engine/app/AApplication.cpp
    ../engine/app/AApplication.hpp
    ../engine/app/IApplication.hpp
    ../engine/core/Logger.cpp
    ../engine/core/Logger.hpp
    ../engine/modulesManager/IModulesManager.hpp
    ../engine/modulesManager/AModulesManager.cpp
    ../engine/modulesManager/AModulesManager.hpp
    ../engine/modulesManager/ModulesManager.cpp
    ../engine/modulesManager/ModulesManager.hpp
)

target_include_directories(sound_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
)

target_link_libraries(sound_test PRIVATE
    cppzmq-static
    ${CMAKE_DL_LIBS}
)

if(UNIX)
    target_link_libraries(sound_test PRIVATE pthread)
endif()

# Make sure SFMLSoundManager and LuaECSManager are built before sound_test
if(TARGET SFMLSoundManager)
    add_dependencies(sound_test SFMLSoundManager)
    
    # Copy the SFMLSoundManager module to the executable directory
    add_custom_command(TARGET sound_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFMLSoundManager>
            $<TARGET_FILE_DIR:sound_test>
        COMMENT "Copying SFMLSoundManager to sound_test directory"
    )
endif()

if(TARGET LuaECSManager)
    add_dependencies(sound_test LuaECSManager)
    
    # Copy the LuaECSManager module to the executable directory
    add_custom_command(TARGET sound_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LuaECSManager>
            $<TARGET_FILE_DIR:sound_test>
        COMMENT "Copying LuaECSManager to sound_test directory"
    )
endif()

# Also copy to lib directory for LD_LIBRARY_PATH
add_custom_command(TARGET sound_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFMLSoundManager>
        ${CMAKE_BINARY_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:LuaECSManager>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying modules to lib directory"
)

# Copy assets to the build directory (if not already done by main target)
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET sound_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:sound_test>/assets"
        COMMENT "Copying assets to sound_test directory"
    )
endif()

# Windows-specific: copy SFML DLLs
if(WIN32 AND TARGET SFML::Audio)
    add_custom_command(TARGET sound_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Audio>
            $<TARGET_FILE_DIR:sound_test>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::System>
            $<TARGET_FILE_DIR:sound_test>
        COMMENT "Copying SFML DLLs for sound_test"
    )
endif()
