include(FetchContent)

# Common dependency interface so all libs and the main project can link against
# the same third-party dependencies.
add_library(project_dependencies INTERFACE)

# CMake 4 removed compatibility with projects declaring cmake_minimum_required < 3.5.
# Some fetched dependencies still do, so provide a minimum policy version for them.
if(POLICY CMP0000)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for third-party subprojects" FORCE)
endif()

# ---- SDL2 (GUI/input/media) ----
find_package(SDL2 QUIET)

if(TARGET SDL2::SDL2)
    target_link_libraries(project_dependencies INTERFACE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(project_dependencies INTERFACE SDL2::SDL2-static)
elseif(SDL2_FOUND)
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(project_dependencies INTERFACE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(project_dependencies INTERFACE ${SDL2_LIBRARIES})
    endif()
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 IMPORTED_TARGET QUIET sdl2)
        if(TARGET PkgConfig::SDL2)
            target_link_libraries(project_dependencies INTERFACE PkgConfig::SDL2)
        endif()
    endif()

    if(NOT TARGET PkgConfig::SDL2)
        message(STATUS "SDL2 not found on system; fetching SDL2 via FetchContent")
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        # SDL2 2.30.x audio/pipewire backend is incompatible with newer PipeWire headers
        # (e.g. PipeWire 1.6.x). Disable PipeWire for fetched SDL2 to keep builds reliable.
        set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
        set(SDL_PIPEWIRE_SHARED OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.30.11
        )
        FetchContent_MakeAvailable(SDL2)
        if(TARGET SDL2::SDL2)
            target_link_libraries(project_dependencies INTERFACE SDL2::SDL2)
        elseif(TARGET SDL2)
            target_link_libraries(project_dependencies INTERFACE SDL2)
        elseif(TARGET SDL2-static)
            target_link_libraries(project_dependencies INTERFACE SDL2-static)
        endif()
    endif()
endif()

# ---- YAML config library (yaml-cpp) ----
find_package(yaml-cpp QUIET)
if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(project_dependencies INTERFACE yaml-cpp::yaml-cpp)
else()
    message(STATUS "yaml-cpp not found on system; fetching yaml-cpp via FetchContent")
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )
    FetchContent_MakeAvailable(yaml-cpp)
    if(TARGET yaml-cpp::yaml-cpp)
        target_link_libraries(project_dependencies INTERFACE yaml-cpp::yaml-cpp)
    elseif(TARGET yaml-cpp)
        target_link_libraries(project_dependencies INTERFACE yaml-cpp)
    endif()
endif()

# ---- Logging library (spdlog) ----
find_package(spdlog QUIET)
if(TARGET spdlog::spdlog)
    target_link_libraries(project_dependencies INTERFACE spdlog::spdlog)
else()
    message(STATUS "spdlog not found on system; fetching spdlog via FetchContent")
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.1
    )
    FetchContent_MakeAvailable(spdlog)
    if(TARGET spdlog::spdlog)
        target_link_libraries(project_dependencies INTERFACE spdlog::spdlog)
    elseif(TARGET spdlog)
        target_link_libraries(project_dependencies INTERFACE spdlog)
    endif()
endif()
