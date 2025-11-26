# GLEWSFML 3D Renderer Module
add_library(GLEWSFMLRenderer SHARED
    GLEWSFMLRenderer.cpp
    GLEWSFMLRenderer.hpp
    ../I3DRenderer.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(GLEWSFMLRenderer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

target_link_libraries(GLEWSFMLRenderer
    GLEW::GLEW
    OpenGL::GL
    cppzmq
    libzmq
    SFML::Graphics
)

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
