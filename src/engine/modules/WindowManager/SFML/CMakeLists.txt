# SFML Window Manager Module
add_library(SFMLWindowManager SHARED
    SFMLWindowManager.cpp
    SFMLWindowManager.hpp
    ../IWindowManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(SFMLWindowManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
)

find_package(cppzmq CONFIG REQUIRED)

target_link_libraries(SFMLWindowManager
    SFML::Window
    SFML::Graphics
    cppzmq-static
)

# Set output name without lib prefix
set_target_properties(SFMLWindowManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "SFMLWindowManager"
)

# Copy SFML and dependency DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET SFMLWindowManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Graphics>
            $<TARGET_FILE_DIR:SFMLWindowManager>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Window>
            $<TARGET_FILE_DIR:SFMLWindowManager>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::System>
            $<TARGET_FILE_DIR:SFMLWindowManager>
        COMMENT "Copying SFML DLLs to module directory"
    )
endif()

# Install rules
install(TARGETS SFMLWindowManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
