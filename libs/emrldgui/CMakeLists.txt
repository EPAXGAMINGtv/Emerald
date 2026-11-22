file(GLOB_RECURSE EMRLDGUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(emrldgui SHARED ${EMRLDGUI_SOURCES})

add_library(Emerald::emrldgui ALIAS emrldgui)

# Ship res/ and the alternate res2/ theme next to the executable.
add_custom_command(TARGET emrldgui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res2"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res2"
    COMMENT "Copying emrldgui resources"
)
target_compile_definitions(emrldgui PUBLIC
    EMERLDGUI_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res"
    EMERLDGUI_RES2_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res2"
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
