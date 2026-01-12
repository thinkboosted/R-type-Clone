# GameEngine Core Library
# This library contains the main game loop and configuration management

add_library(GameEngine STATIC
    GameEngine.cpp
    GameEngine.hpp
    ../app/AApplication.cpp
    ../app/AApplication.hpp
    ../app/IApplication.hpp
    ../modulesManager/IModulesManager.hpp
    ../modulesManager/AModulesManager.cpp
    ../modulesManager/AModulesManager.hpp
    ../modulesManager/ModulesManager.cpp
    ../modulesManager/ModulesManager.hpp
)

# Include directories
target_include_directories(GameEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/modules
    ${CMAKE_SOURCE_DIR}/src/engine/types
)

# Find dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

# ZeroMQ (for message broker)
find_package(ZeroMQ CONFIG QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZeroMQ REQUIRED libzmq)
endif()

find_package(cppzmq CONFIG QUIET)

# Link dependencies
target_link_libraries(GameEngine PUBLIC
    nlohmann_json::nlohmann_json
    sol2::sol2
    ${LUA_LIBRARIES}
    cppzmq-static
)

# Platform-specific libraries
if(WIN32)
    # Windows dynamic loading is built-in
else()
    # Linux/Unix needs dl for dynamic loading
    target_link_libraries(GameEngine PUBLIC ${CMAKE_DL_LIBS})
endif()

# Compiler flags
target_compile_features(GameEngine PUBLIC cxx_std_17)

# Add warnings
if(MSVC)
    target_compile_options(GameEngine PRIVATE /W4)
else()
    target_compile_options(GameEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
