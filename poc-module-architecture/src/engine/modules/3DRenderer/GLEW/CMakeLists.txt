# GLEW 3D Renderer Module
add_library(GLEWRenderer SHARED
    GLEWRenderer.cpp
    GLEWRenderer.hpp
    ../I3DRenderer.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(GLEWRenderer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

target_link_libraries(GLEWRenderer
    GLEW::GLEW
    OpenGL::GL
    cppzmq
    libzmq
    SFML::Window
    SFML::System
)

# Set output name without lib prefix
set_target_properties(GLEWRenderer PROPERTIES
    PREFIX ""
    OUTPUT_NAME "GLEWRenderer"
)

# Install rules
install(TARGETS GLEWRenderer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
