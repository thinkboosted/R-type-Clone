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

# Try find_package with CONFIG first (for vcpkg), fall back to pkg-config
find_package(ZeroMQ CONFIG QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZeroMQ REQUIRED libzmq)
    set(ZeroMQ_LINK_LIBS ${ZeroMQ_LINK_LIBRARIES})
else()
    set(ZeroMQ_LINK_LIBS libzmq)
endif()

find_package(cppzmq CONFIG QUIET)
if(NOT cppzmq_FOUND)
    # cppzmq is header-only
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include
    )
endif()

if(NOT WIN32)
    find_package(X11 REQUIRED)
endif()

target_link_libraries(GLEWSFMLRenderer
    GLEW::GLEW
    OpenGL::GL
    SFML::Graphics
    ${ZeroMQ_LINK_LIBS}
)

target_include_directories(GLEWSFMLRenderer PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
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
