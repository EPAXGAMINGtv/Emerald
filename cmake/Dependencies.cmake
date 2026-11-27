include(FetchContent)

option(EMERALD_ENABLE_GUI "Fetch SDL2/SDL2_ttf for emrldgui" ON)
option(EMERALD_ENABLE_ALL_WARNINGS "Enable the most comprehensive warning set the compiler supports" ON)
option(EMERALD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(project_warnings INTERFACE)
add_library(cfg_deps INTERFACE)
add_library(gui_deps INTERFACE)

add_library(project_dependencies INTERFACE)
target_link_libraries(project_dependencies INTERFACE
    project_warnings
    cfg_deps
    gui_deps
)

if(POLICY CMP0000)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for third-party subprojects" FORCE)
endif()

if(MSVC)
    target_compile_options(project_warnings INTERFACE
        /W4
        /permissive-
        /w14242 /w14254 /w14263 /w14265 /w14287 /we4289
        /w14296 /w14311 /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
        $<$<BOOL:${EMERALD_WARNINGS_AS_ERRORS}>:/WX>
    )
else()
    # Baseline that every GCC/Clang build gets.
    target_compile_options(project_warnings INTERFACE
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wshadow>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wformat=2>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wsign-conversion>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wcast-qual>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wold-style-cast>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wnull-dereference>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wdouble-promotion>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Woverloaded-virtual>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wnon-virtual-dtor>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra-semi>
        $<$<COMPILE_LANGUAGE:CXX>:-Wzero-as-null-pointer-constant>
        $<$<BOOL:${EMERALD_WARNINGS_AS_ERRORS}>:-Werror>
    )

    if(EMERALD_ENABLE_ALL_WARNINGS)
        target_compile_options(project_warnings INTERFACE
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wunused-but-set-variable>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wunused-but-set-parameter>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wundef>
            $<$<COMPILE_LANGUAGE:CXX>:-Wswitch-default>
            $<$<COMPILE_LANGUAGE:CXX>:-Wswitch-enum>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wswitch-unreachable>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wimplicit-fallthrough=5>
            $<$<COMPILE_LANGUAGE:CXX>:-Wlogical-op>
            $<$<COMPILE_LANGUAGE:CXX>:-Wlogical-not-parentheses>
            $<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-cond>
            $<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-branches>
            $<$<COMPILE_LANGUAGE:CXX>:-Wrestrict>
            $<$<COMPILE_LANGUAGE:CXX>:-Wformat-overflow=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wformat-truncation=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wstringop-overflow=4>
            $<$<COMPILE_LANGUAGE:CXX>:-Warray-bounds=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wshift-negative-value>
            $<$<COMPILE_LANGUAGE:CXX>:-Wshift-overflow=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wuse-after-free=3>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdangling-pointer=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wredundant-decls>
            $<$<COMPILE_LANGUAGE:CXX>:-Wmissing-include-dirs>
            $<$<COMPILE_LANGUAGE:CXX>:-Wmissing-declarations>
            $<$<COMPILE_LANGUAGE:CXX>:-Wmissing-noreturn>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdisabled-optimization>
            $<$<COMPILE_LANGUAGE:CXX>:-Wuseless-cast>
            $<$<COMPILE_LANGUAGE:CXX>:-Wsuggest-override>
            $<$<COMPILE_LANGUAGE:CXX>:-Wcatch-value=3>
            $<$<COMPILE_LANGUAGE:CXX>:-Wconditionally-supported>
            $<$<COMPILE_LANGUAGE:CXX>:-Wctor-dtor-privacy>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdeprecated-copy>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdeprecated-enum-enum-conversion>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdeprecated-enum-float-conversion>
            $<$<COMPILE_LANGUAGE:CXX>:-Wsign-promo>
            $<$<COMPILE_LANGUAGE:CXX>:-Wfloat-equal>
            $<$<COMPILE_LANGUAGE:CXX>:-Wfloat-conversion>
            $<$<COMPILE_LANGUAGE:CXX>:-Wnonnull-compare>
            $<$<COMPILE_LANGUAGE:CXX>:-Wsizeof-pointer-memaccess>
            $<$<COMPILE_LANGUAGE:CXX>:-Wnoexcept>
            $<$<COMPILE_LANGUAGE:CXX>:-Wattributes>
            $<$<COMPILE_LANGUAGE:CXX>:-Wbuiltin-macro-redefined>
            $<$<COMPILE_LANGUAGE:CXX>:-Wdangling-else>
            $<$<COMPILE_LANGUAGE:CXX>:-Wempty-body>
            $<$<COMPILE_LANGUAGE:CXX>:-Wignored-qualifiers>
            $<$<COMPILE_LANGUAGE:CXX>:-Winit-self>
            $<$<COMPILE_LANGUAGE:CXX>:-Wmisleading-indentation>
            $<$<COMPILE_LANGUAGE:CXX>:-Wparentheses>
            $<$<COMPILE_LANGUAGE:CXX>:-Wtype-limits>
            $<$<COMPILE_LANGUAGE:CXX>:-Wunknown-pragmas>
            $<$<COMPILE_LANGUAGE:CXX>:-Wwrite-strings>
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(project_warnings INTERFACE
            $<$<COMPILE_LANGUAGE:C,CXX>:-Weverything>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-c++98-compat>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-c++98-compat-pedantic>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-padded>
            $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-unsafe-buffer-usage>
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(project_warnings INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:-Wcast-align=strict>
        )
    endif()
endif()

if(EMERALD_ENABLE_GUI)
# The system SDL2 is built without a Wayland video backend here: SDL_CreateWindow()
# succeeds with the offscreen driver, but niri never gets a mapped surface.
# Build SDL2 with both Wayland and X11 video backends via FetchContent so windows
# show up on Wayland (e.g. niri) as well as on X11 desktops.
message(STATUS "Building SDL2 with Wayland and X11 video backends via FetchContent")
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_X11 ON CACHE BOOL "" FORCE)
set(SDL_WAYLAND ON CACHE BOOL "" FORCE)
set(SDL_WAYLAND_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_WAYLAND_LIBDECOR OFF CACHE BOOL "" FORCE)
set(SDL_WAYLAND_QT_TOUCH OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL2
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.11.tar.gz
    URL_HASH SHA256=cc6136dd964854e8846c679703322f3e2a341d27a06a53f8b3f642c26f1b0cfd
)
FetchContent_MakeAvailable(SDL2)

# SDL2 2.30.11 has one discarded-qualifiers warning in its own qsort code.
# Third-party sources are outside our zero-warning policy -> silence it locally.
if(NOT MSVC)
    foreach(sdl_target SDL2 SDL2-static)
        if(TARGET ${sdl_target})
            target_compile_options(${sdl_target} PRIVATE
                $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers>
            )
        endif()
    endforeach()
endif()

if(TARGET SDL2::SDL2)
    target_link_libraries(gui_deps INTERFACE SDL2::SDL2)
elseif(TARGET SDL2)
    target_link_libraries(gui_deps INTERFACE SDL2)
elseif(TARGET SDL2-static)
    target_link_libraries(gui_deps INTERFACE SDL2-static)
endif()

# SDL headers are third-party: never let them trip our warning policy in our files.
foreach(_sdl_target SDL2::SDL2 SDL2 SDL2-static)
    if(TARGET ${_sdl_target})
        get_target_property(_sdl_alias ${_sdl_target} ALIASED_TARGET)
        if(NOT _sdl_alias)
            set_target_properties(${_sdl_target} PROPERTIES SYSTEM TRUE)
        endif()
    endif()
endforeach()
if(DEFINED SDL2_SOURCE_DIR)
    target_include_directories(gui_deps SYSTEM INTERFACE "${SDL2_SOURCE_DIR}/include")
endif()

find_package(SDL2_ttf QUIET)
if(TARGET SDL2_ttf::SDL2_ttf)
    target_link_libraries(gui_deps INTERFACE SDL2_ttf::SDL2_ttf)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_TTF IMPORTED_TARGET QUIET SDL2_ttf)
        if(TARGET PkgConfig::SDL2_TTF)
            target_link_libraries(gui_deps INTERFACE PkgConfig::SDL2_TTF)
        endif()
    endif()

    if(NOT TARGET PkgConfig::SDL2_TTF)
        message(STATUS "SDL2_ttf not found on system; fetching SDL2_ttf via FetchContent")
        set(SDL2TTF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
        set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
        set(SDL2TTF_VENDORED OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            SDL_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-2.20.2
            GIT_SUBMODULES ""
        )
        FetchContent_MakeAvailable(SDL_ttf)

        if(TARGET SDL2_ttf::SDL2_ttf)
            target_link_libraries(gui_deps INTERFACE SDL2_ttf::SDL2_ttf)
        elseif(TARGET SDL2_ttf)
            target_link_libraries(gui_deps INTERFACE SDL2_ttf)
        endif()
    endif()
endif()

#third party headers are system headers, so their warnings stay out of our files ->
foreach(_ttf_target SDL2_ttf::SDL2_ttf SDL2_ttf)
    if(TARGET ${_ttf_target})
        get_target_property(_ttf_alias ${_ttf_target} ALIASED_TARGET)
        if(NOT _ttf_alias)
            set_target_properties(${_ttf_target} PROPERTIES SYSTEM TRUE)
        endif()
    endif()
endforeach()
if(DEFINED SDL_ttf_SOURCE_DIR)
    target_include_directories(gui_deps SYSTEM INTERFACE "${SDL_ttf_SOURCE_DIR}")
endif()
endif()

find_package(yaml-cpp QUIET)
if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(cfg_deps INTERFACE yaml-cpp::yaml-cpp)
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
        target_link_libraries(cfg_deps INTERFACE yaml-cpp::yaml-cpp)
    elseif(TARGET yaml-cpp)
        target_link_libraries(cfg_deps INTERFACE yaml-cpp)
    endif()

    #yaml-cpp 0.8.0 uses uint16_t without including <cstdint> on newer compilers ->
    if(TARGET yaml-cpp)
        if(MSVC)
            target_compile_options(yaml-cpp PRIVATE /FIcstdint)
        else()
            target_compile_options(yaml-cpp PRIVATE -include cstdint)
        endif()
    endif()
endif()

# yaml-cpp is third-party: its headers must not trigger our warning policy.
if(TARGET yaml-cpp)
    set_target_properties(yaml-cpp PROPERTIES SYSTEM TRUE)
endif()
if(DEFINED yaml-cpp_SOURCE_DIR)
    target_include_directories(cfg_deps SYSTEM INTERFACE "${yaml-cpp_SOURCE_DIR}/include")
endif()

find_package(spdlog QUIET)
if(TARGET spdlog::spdlog)
    target_link_libraries(cfg_deps INTERFACE spdlog::spdlog)
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
        target_link_libraries(cfg_deps INTERFACE spdlog::spdlog)
    elseif(TARGET spdlog)
        target_link_libraries(cfg_deps INTERFACE spdlog)
    endif()
endif()

#third party headers are system headers, so their warnings stay out of our files ->
if(TARGET spdlog)
    set_target_properties(spdlog PROPERTIES SYSTEM TRUE)
endif()
if(DEFINED spdlog_SOURCE_DIR)
    target_include_directories(cfg_deps SYSTEM INTERFACE "${spdlog_SOURCE_DIR}/include")
endif()
