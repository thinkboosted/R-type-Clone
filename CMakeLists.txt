cmake_minimum_required(VERSION 3.20)
project(RTypePOC VERSION 1.0.0 LANGUAGES CXX)

# Support Conan fallback - REMOVED (Replaced by FetchContent in cmake/Dependencies.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Dependencies Management ---
include(cmake/Dependencies.cmake)

# Lua is needed globally because ecs.hpp includes sol2, which requires Lua headers
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# --- Compatibility Aliases (FetchContent vs Vcpkg) ---
if (NOT TARGET SFML::Graphics AND TARGET sfml-graphics)
    add_library(SFML::Graphics ALIAS sfml-graphics)
    add_library(SFML::Window ALIAS sfml-window)
    add_library(SFML::System ALIAS sfml-system)
    add_library(SFML::Network ALIAS sfml-network)
    add_library(SFML::Audio ALIAS sfml-audio)
endif()

if (NOT TARGET BulletDynamics AND TARGET Bullet3::BulletDynamics)
     # Vcpkg exports Bullet3::BulletDynamics, plain cmake might just do BulletDynamics.
     # Let's standardize on what the sub-modules use: BulletDynamics
     # If fetched content provides 'BulletDynamics' target directly, we are good.
endif()

# Find System/Graphics Libs (Usually provided by OS, hard to FetchContent properly)
message(STATUS "Looking for GLEW...")
find_package(GLEW QUIET)
if(NOT GLEW_FOUND)
    message(WARNING "GLEW not found via find_package. On Linux, try: sudo apt install libglew-dev")
endif()

message(STATUS "Looking for OpenGL...")
find_package(OpenGL QUIET)
if(NOT OpenGL_FOUND)
    message(WARNING "OpenGL not found. On Linux, try: sudo apt install libgl1-mesa-dev")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories for engine
include_directories(
    ${CMAKE_SOURCE_DIR}/src/engine
)

# Add core engine library
add_subdirectory(src/engine/core)

# Add subdirectories for modules
if(SFML_FOUND OR TARGET sfml-system)
    add_subdirectory(src/engine/modules/WindowManager/SFML)
    add_subdirectory(src/engine/modules/SoundManager/SFML)
else()
    message(WARNING "Skipping SFML Window Manager module (SFML not found)")
    message(WARNING "Skipping SFML Sound Manager module")
endif()

if(GLEW_FOUND AND OpenGL_FOUND)
    add_subdirectory(src/engine/modules/Renderer/GLEWSFML)
else()
    message(WARNING "Skipping GLEW Renderer module")
endif()

add_subdirectory(src/engine/modules/ECSManager/LuaECSManager)
add_subdirectory(src/engine/modules/ECSSavesManager/BasicECSSavesManager)

if(Bullet_FOUND OR TARGET BulletDynamics)
    add_subdirectory(src/engine/modules/PhysicEngine/Bullet)
else()
    message(WARNING "Skipping Bullet Physics Engine module")
endif()


add_subdirectory(src/engine/modules/NetworkManager)

# Add subdirectory for game executable
add_subdirectory(src/game)

# Add subdirectory for sound test executable
add_subdirectory(src/test_sound)
