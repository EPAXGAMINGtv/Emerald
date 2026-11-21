add_library(emrldgui SHARED
    src/Gui.cpp
)

add_library(Emerald::emrldgui ALIAS emrldgui)

target_include_directories(emrldgui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(emrldgui
    PUBLIC
        cfg
        eemerge
        project_dependencies
)

set_target_properties(emrldgui PROPERTIES
    EXPORT_NAME emrldgui
)
