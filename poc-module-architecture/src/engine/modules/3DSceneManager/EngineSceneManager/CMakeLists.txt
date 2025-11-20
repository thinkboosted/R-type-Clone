# RType Scene Manager Module
add_library(EngineSceneManager SHARED
    EngineSceneManager.cpp
    EngineSceneManager.hpp
    ../I3DSceneManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(EngineSceneManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
)

target_link_libraries(EngineSceneManager
    cppzmq
    libzmq
)

# Set output name without lib prefix
set_target_properties(EngineSceneManager PROPERTIES
    PREFIX ""
    OUTPUT_NAME "EngineSceneManager"
)

# Install rules
install(TARGETS EngineSceneManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
