# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge_thread_sanitizer EXCLUDE_FROM_ALL ${HEADERS} ${SRCS} ${MAIN_SRCS} ${FOR_IDE} ${VERSION_FILE})
add_dependencies(malbolge_thread_sanitizer gen_version)

target_compile_features(malbolge_thread_sanitizer PUBLIC cxx_std_20)
set_target_properties(malbolge_thread_sanitizer PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_thread_sanitizer PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

target_include_directories(malbolge_thread_sanitizer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_thread_sanitizer
    PUBLIC Threads::Threads
)

target_compile_options(malbolge_thread_sanitizer
    PRIVATE -fsanitize=thread
)

target_link_options(malbolge_thread_sanitizer
    PRIVATE -fsanitize=thread
)
