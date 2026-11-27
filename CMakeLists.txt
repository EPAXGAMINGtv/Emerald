cmake_minimum_required(VERSION 3.25)

project(Emerald VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR};$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/Dependencies.cmake)

add_subdirectory(libs)

file(GLOB_RECURSE EMERALD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(Emerald ${EMERALD_SOURCES})

target_link_libraries(Emerald
    PRIVATE
        emrldgui
        eemerge
        cfg
        flatpak
        project_dependencies
)

target_include_directories(Emerald PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(EMERALD_BUILD_TESTS "Build the terminal test binary" ON)
if(EMERALD_BUILD_TESTS)
    # Target is intentionally named "test" (no enable_testing(), which would
    # reserve that name for CTest). Run it with: cmake --build build --target check
    add_executable(test
        tests/test_main.cpp
        tests/test_config.cpp
        tests/test_config_exhaustive.cpp
        tests/test_config_manager.cpp
        tests/test_gui_core.cpp
        tests/test_gui_advanced.cpp
        tests/test_gui_widgets.cpp
        tests/test_config_schema.cpp
        tests/test_config_stack.cpp
        tests/test_config_environment.cpp
        tests/test_config_diff.cpp
        tests/test_config_migration.cpp
        tests/test_flatpak.cpp
        tests/test_flatpak_repositories.cpp
        tests/test_flatpak_applications.cpp
        tests/test_flatpak_icons.cpp
        tests/test_flatpak_operations.cpp
        tests/test_flatpak_groups.cpp
        tests/test_flatpak_streaming.cpp
        tests/test_flatpak_terminal.cpp
        tests/test_flatpak_default_repos.cpp
    )
    target_link_libraries(test PRIVATE cfg emrldgui flatpak project_warnings)

    add_custom_target(check
        COMMAND $<TARGET_FILE:test>
        DEPENDS test
        COMMENT "Running Emerald terminal tests"
        VERBATIM
    )
endif()

install(TARGETS Emerald
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Ship the third-party shared objects that were built via FetchContent next to
# our libraries, so the installed tree is self-contained.
install(DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILES_MATCHING PATTERN "*.so*"
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/EmeraldConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmeraldConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/EmeraldConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Emerald
)
