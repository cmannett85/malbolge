# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_library(malbolge_lib_coverage ${HEADERS} ${SRCS} ${FOR_IDE} ${VERSION_FILE})
add_dependencies(malbolge_lib_coverage gen_version)

target_compile_features(malbolge_lib_coverage PUBLIC cxx_std_20)
set_target_properties(malbolge_lib_coverage PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_lib_coverage PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

target_include_directories(malbolge_lib_coverage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_lib_coverage
    PUBLIC Threads::Threads
)

target_compile_options(malbolge_lib_coverage
    PRIVATE --coverage
)

target_link_options(malbolge_lib_coverage
    PRIVATE --coverage
)
