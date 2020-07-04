# Taken from https://stackoverflow.com/a/50104093/498437
#

find_package(Git REQUIRED)

set(VERSION_FILE_TMP ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.tmp)

execute_process(
    OUTPUT_VARIABLE   GIT_REV
    COMMAND           ${GIT_EXECUTABLE} rev-parse -q HEAD
    WORKING_DIRECTORY ${ORIG_BASE_PATH}
)

message(STATUS "Git revision: ${GIT_REV}")

string(STRIP ${GIT_REV} GIT_REV)

configure_file(
    ${ORIG_BASE_PATH}/cmake/version.hpp.in
    ${VERSION_FILE_TMP}
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VERSION_FILE_TMP} ${VERSION_FILE}
)
