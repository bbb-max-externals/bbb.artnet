cmake_minimum_required(VERSION 3.19)
project(bbb.artnet)

enable_testing()

execute_process(
    COMMAND git rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_COMMIT_COUNT)
    set(GIT_COMMIT_COUNT 1)
endif()
set(BBB_VERSION_MAJOR 0)
set(BBB_VERSION_MINOR 0)
set(BBB_VERSION_PATCH ${GIT_COMMIT_COUNT})
set(BBB_VERSION "${BBB_VERSION_MAJOR}.${BBB_VERSION_MINOR}.${BBB_VERSION_PATCH}")

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/source/bbb/version.h
    "#pragma once\n"
    "#define BBB_ARTNET_VERSION \"${BBB_VERSION}\"\n"
)

if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
    endif()
endif()

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/min-api)
set(C74_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/externals)

MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/libartnet)

SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach(project_dir ${PROJECT_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
    endif()
endforeach()

set(CMAKE_SUPPRESS_REGENERATION true)
