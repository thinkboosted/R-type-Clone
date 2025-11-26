cmake_minimum_required(VERSION 3.20)
project(RTypePOC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Copy assets directory to build directory
if (EXISTS "${CMAKE_SOURCE_DIR}/assets")
  add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets to ${CMAKE_BINARY_DIR}/assets"
  )
  if (TARGET rtype_game)
    add_dependencies(rtype_game copy_assets)
  endif()
endif()

# Find required packages
message(STATUS "Looking for SFML...")
find_package(SFML COMPONENTS Window Graphics)
if(NOT SFML_FOUND)
    message(WARNING "SFML not found. Please install via vcpkg: vcpkg install sfml")
endif()

message(STATUS "Looking for GLEW...")
find_package(GLEW)
if(NOT GLEW_FOUND)
    message(WARNING "GLEW not found. Please install via vcpkg: vcpkg install glew")
endif()

message(STATUS "Looking for OpenGL...")
find_package(OpenGL)
if(NOT OpenGL_FOUND)
    message(WARNING "OpenGL not found. Please install via vcpkg: vcpkg install opengl")
endif()

message(STATUS "Looking for ZeroMQ...")
find_package(cppzmq CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
if(NOT cppzmq_FOUND OR NOT ZeroMQ_FOUND)
    message(WARNING "ZeroMQ or cppzmq not found. Please install via vcpkg: vcpkg install zeromq cppzmq")
endif()

if(NOT TARGET libzmq AND TARGET libzmq-static)
    add_library(libzmq ALIAS libzmq-static)
endif()

message(STATUS "Looking for Bullet...")
find_package(Bullet CONFIG REQUIRED PATHS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-mingw-dynamic/share/bullet3")
if(NOT Bullet_FOUND)
    message(WARNING "Bullet not found. Please install via vcpkg: vcpkg install bullet3")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories for engine
include_directories(
    ${CMAKE_SOURCE_DIR}/src/engine
)

# Add subdirectories for modules
if(SFML_FOUND)
    add_subdirectory(src/engine/modules/WindowManager/SFML)
else()
    message(WARNING "Skipping SFML Window Manager module")
endif()

if(GLEW_FOUND AND OpenGL_FOUND)
    add_subdirectory(src/engine/modules/Renderer/GLEW)
else()
    message(WARNING "Skipping GLEW Renderer module")
endif()

add_subdirectory(src/engine/modules/LuaECS)

if(Bullet_FOUND)
    add_subdirectory(src/engine/modules/PhysicEngine/Bullet)
else()
    message(WARNING "Skipping Bullet Physics Engine module")
endif()

# Add subdirectory for game executable
add_subdirectory(src/game)
