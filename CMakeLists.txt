cmake_minimum_required(VERSION 3.19)
project(bbb.artnet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

enable_testing()

set(BBB_ARTNET_BUILD_TESTS ON CACHE BOOL "Build bbb-artnet unit tests" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/bbb-artnet)

set(BBB_SACN_BUILD_TESTS ON CACHE BOOL "Build bbb-sacn unit tests" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/bbb-sacn)

add_executable(bbb_universe_remap_tests
    tests/universe_remap_tests.cpp
)
target_include_directories(bbb_universe_remap_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)
add_test(NAME bbb_universe_remap_tests COMMAND bbb_universe_remap_tests)

add_custom_target(generate_version ALL
    COMMAND ${CMAKE_COMMAND}
        -DWORKING_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DVERSION_FILE=${CMAKE_CURRENT_SOURCE_DIR}/source/bbb/version.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.cmake
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

if(WIN32)
    link_libraries(ws2_32 iphlpapi)
endif()

SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach(project_dir ${PROJECT_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
        if(TARGET ${project_dir})
            add_dependencies(${project_dir} generate_version)
        endif()
    endif()
endforeach()

set(CMAKE_SUPPRESS_REGENERATION true)
