cmake_minimum_required(VERSION 3.25)

project(Emerald VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output shared libraries and the main binary into predictable locations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/Dependencies.cmake)

add_subdirectory(libs)

add_executable(Emerald src/main.cpp
        libs/eemerge/include/eemerge/Emerge.cpp
        libs/eemerge/include/eemerge/Emerge.h)

target_link_libraries(Emerald
    PRIVATE
        emrldgui
        eemerge
        cfg
        project_dependencies
)

target_include_directories(Emerald PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
