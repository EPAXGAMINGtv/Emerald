file(GLOB_RECURSE EMRLDGUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(emrldgui SHARED ${EMRLDGUI_SOURCES})

add_library(Emerald::emrldgui ALIAS emrldgui)

# Ship the CSS theme in res/ next to the executable.
# A custom target (instead of POST_BUILD) also refreshes these files when only
# CSS changes and the library itself does not need to be rebuilt.
file(GLOB_RECURSE EMRLDGUI_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/*"
)
add_custom_target(emrldgui_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res"
    DEPENDS ${EMRLDGUI_RESOURCE_FILES}
    COMMENT "Copying emrldgui resources"
    VERBATIM
)
add_dependencies(emrldgui emrldgui_resources)
target_compile_definitions(emrldgui PUBLIC
    EMERLDGUI_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res"
)

target_include_directories(emrldgui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

#project_dependencies provides SDL2, SDL2_ttf and spdlog; no cfg/eemerge dependency here ->
target_link_libraries(emrldgui
    PUBLIC
        project_dependencies
)

set_target_properties(emrldgui PROPERTIES
    EXPORT_NAME emrldgui
)
