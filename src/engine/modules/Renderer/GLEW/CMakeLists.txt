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

find_package(ZeroMQ CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

if(NOT WIN32)
    find_package(X11 REQUIRED)
endif()

target_link_libraries(GLEWRenderer
    GLEW::GLEW
    OpenGL::GL
    libzmq
    cppzmq
)

if(NOT WIN32)
    target_link_libraries(GLEWRenderer X11::X11)
endif()

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
