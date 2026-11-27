# EmeraldConfig.cmake
#
# Provides imported targets for the installed Emerald libraries:
#   Emerald::cfg, Emerald::eemerge, Emerald::emrldgui, Emerald::flatpak
#
# Third-party dependencies are optional here: the shared objects already carry
# their NEEDED entries. When the packages are found, their targets are attached
# so consumers also get the matching include directories.

get_filename_component(_Emerald_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

include(CMakeFindDependencyMacro)
find_package(spdlog QUIET)
find_package(yaml-cpp QUIET)
find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)

set(_Emerald_DEPENDENCIES)
foreach(_emerald_dep IN ITEMS spdlog::spdlog yaml-cpp::yaml-cpp SDL2::SDL2 SDL2_ttf::SDL2_ttf)
    if(TARGET ${_emerald_dep})
        list(APPEND _Emerald_DEPENDENCIES ${_emerald_dep})
    endif()
endforeach()

function(_emerald_import_target _name _library)
    if(TARGET Emerald::${_name})
        return()
    endif()

    set(_libdir "${_Emerald_PREFIX}/lib")
    if(NOT EXISTS "${_libdir}/${_library}")
        set(_libdir "${_Emerald_PREFIX}/lib64")
    endif()

    add_library(Emerald::${_name} UNKNOWN IMPORTED)
    set_target_properties(Emerald::${_name} PROPERTIES
        IMPORTED_LOCATION "${_libdir}/${_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${_Emerald_PREFIX}/include"
        INTERFACE_LINK_DIRECTORIES "${_libdir}"
        INTERFACE_LINK_LIBRARIES "${_Emerald_DEPENDENCIES}"
    )
endfunction()

_emerald_import_target(cfg          "libcfg.so")
_emerald_import_target(eemerge      "libeemerge.so")
_emerald_import_target(emrldgui     "libemrldgui.so")
_emerald_import_target(flatpak      "libemerald_flatpak.so")
