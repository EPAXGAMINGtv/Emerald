add_library(eemerge SHARED
    src/eemerge/Emerge.cpp
    src/eemerge/CMDline.cpp
)

add_library(Emerald::eemerge ALIAS eemerge)

target_include_directories(eemerge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(eemerge
    PUBLIC
        cfg
        project_dependencies
)

set_target_properties(eemerge PROPERTIES
    EXPORT_NAME eemerge
)
