file(GLOB_RECURSE CFG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
list(FILTER CFG_SOURCES EXCLUDE REGEX ".*/Log\\.(cpp|hpp)$")

add_library(cfg SHARED ${CFG_SOURCES})

add_library(Emerald::cfg ALIAS cfg)

target_include_directories(cfg
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cfg
    PUBLIC
        project_dependencies
)

set_target_properties(cfg PROPERTIES
    OUTPUT_NAME cfg
    EXPORT_NAME cfg
)
