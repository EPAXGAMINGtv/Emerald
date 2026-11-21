file(GLOB_RECURSE EEMERGE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(eemerge SHARED ${EEMERGE_SOURCES})

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
