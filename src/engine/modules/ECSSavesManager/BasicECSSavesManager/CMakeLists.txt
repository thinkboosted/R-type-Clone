add_library(ECSSavesManager SHARED
    BasicECSSavesManager.cpp
    BasicECSSavesManager.hpp
    ../IECSSavesManager.hpp
    ../../IModule.hpp
    ../../AModule.hpp
    ../../AModule.cpp
)

target_include_directories(ECSSavesManager PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
)

find_package(cppzmq CONFIG REQUIRED)

target_link_libraries(ECSSavesManager
    cppzmq
)

set_target_properties(ECSSavesManager PROPERTIES PREFIX "")
