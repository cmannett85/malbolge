# Copyright Cam Mannett 2020
#
# See LICENSE file
#

list(APPEND FOR_IDE
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/documentation_script.cmake)

add_custom_target(documentation
    COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/documentation_script.cmake
)
add_dependencies(documentation gen_version)
