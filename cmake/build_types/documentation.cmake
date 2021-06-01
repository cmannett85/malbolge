# Copyright Cam Mannett 2020
#
# See LICENSE file
#

if(DOCS_ONLY)
    message(STATUS "Documentation-only build")
endif()

set(DOCS_FOR_IDE
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/documentation_script.cmake
)

set(README_API_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/docs/README_API.md
)

add_custom_target(documentation
    COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_types/documentation_script.cmake
    SOURCES ${DOCS_FOR_IDE} ${FOR_IDE}
)
add_dependencies(documentation gen_version)

# We have touch the generated API readme because CPack configuration will fail
# if it is not there.  set_source_files_properties cannot be used in a script
# so that's set here too
file(TOUCH ${README_API_PATH})
set_source_files_properties(${README_API_PATH} PROPERTIES GENERATED TRUE)
