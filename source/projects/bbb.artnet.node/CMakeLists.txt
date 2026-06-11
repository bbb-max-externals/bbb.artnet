cmake_minimum_required(VERSION 3.19)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories(
    "${C74_INCLUDES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../source"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/bbb-artnet/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/osc/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/osc/third-party/asio/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/osc/third-party/bit_by_bit/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/osc/third-party/oscpp/include"
)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

set(HELP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.maxhelp")
set(HELP_DEST "${CMAKE_CURRENT_SOURCE_DIR}/../../../help/${PROJECT_NAME}.maxhelp")
if(EXISTS "${HELP_SOURCE}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HELP_SOURCE}" "${HELP_DEST}"
    )
endif()

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)
