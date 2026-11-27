file(GLOB_RECURSE FLATPAK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(flatpak SHARED ${FLATPAK_SOURCES})

add_library(Emerald::flatpak ALIAS flatpak)
add_library(flatpak::flatpak ALIAS flatpak)

target_include_directories(flatpak
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(TARGET spdlog::spdlog)
    target_link_libraries(flatpak PUBLIC spdlog::spdlog)
elseif(TARGET spdlog)
    target_link_libraries(flatpak PUBLIC spdlog)
endif()

target_link_libraries(flatpak PRIVATE project_warnings)

set_target_properties(flatpak PROPERTIES
    OUTPUT_NAME emerald_flatpak
    EXPORT_NAME flatpak
)

install(TARGETS flatpak
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
