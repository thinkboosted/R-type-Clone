# GLEWSFML 3D Renderer Module
add_library(GLEWSFMLRenderer SHARED
    GLEWSFMLRenderer.cpp
    GLEWSFMLRenderer.hpp
    RenderStructs.hpp
    ResourceManager.cpp
    ResourceManager.hpp
    ParticleSystem.cpp
    ParticleSystem.hpp
    ../I3DRenderer.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(GLEWSFMLRenderer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

find_package(cppzmq CONFIG REQUIRED)

if(NOT WIN32)
    find_package(X11 REQUIRED)
endif()

target_link_libraries(GLEWSFMLRenderer
    GLEW::GLEW
    OpenGL::GL
    SFML::Graphics
    cppzmq-static
)

if(NOT WIN32)
    target_link_libraries(GLEWSFMLRenderer X11::X11)
endif()

# Set output name without lib prefix
set_target_properties(GLEWSFMLRenderer PROPERTIES
    PREFIX ""
    OUTPUT_NAME "GLEWSFMLRenderer"
)

# Install rules
install(TARGETS GLEWSFMLRenderer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
