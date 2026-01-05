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
    # cppzmq is header-only, try to find the headers
    find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
        PATHS /usr/include /usr/local/include /opt/include
        PATH_SUFFIXES cppzmq
    )
    if(NOT CPPZMQ_INCLUDE_DIR)
        message(WARNING "cppzmq headers not found, hoping they're in standard paths")
    endif()
endif()

target_link_libraries(SFMLWindowManager
    SFML::Window
    SFML::Graphics
    ${ZeroMQ_LINK_LIBS}
)

# Add include directories for ZeroMQ and cppzmq
target_include_directories(SFMLWindowManager PRIVATE
    ${ZeroMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIR}
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
