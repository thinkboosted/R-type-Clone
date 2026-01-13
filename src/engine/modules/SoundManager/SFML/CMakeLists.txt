# SFML Sound Manager Module
add_library(SFMLSoundManager SHARED
    SFMLSoundManager.cpp
    SFMLSoundManager.hpp
    ../ISoundManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
    ../../../core/Logger.cpp
)

target_include_directories(SFMLSoundManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
)

find_package(cppzmq CONFIG REQUIRED)

target_link_libraries(SFMLSoundManager
    SFML::Audio
    SFML::System
    cppzmq-static
)

# Set output name without lib prefix
set_target_properties(SFMLSoundManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "SFMLSoundManager"
)

# Copy SFML and dependency DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET SFMLSoundManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Audio>
            $<TARGET_FILE_DIR:SFMLSoundManager>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::System>
            $<TARGET_FILE_DIR:SFMLSoundManager>
        COMMENT "Copying SFML Audio and dependency DLLs to module directory"
    )
endif()

# Install rules
install(TARGETS SFMLSoundManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
