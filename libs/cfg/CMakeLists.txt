add_library(cfg SHARED
    src/Config.cpp
    src/Log.cpp
)

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
