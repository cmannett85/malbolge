# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge_address_sanitizer EXCLUDE_FROM_ALL ${MAIN_SRCS})
add_dependencies(malbolge_address_sanitizer gen_version malbolge_lib)

target_compile_features(malbolge_address_sanitizer PUBLIC cxx_std_20)
set_target_properties(malbolge_address_sanitizer PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_address_sanitizer PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

target_include_directories(malbolge_address_sanitizer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_address_sanitizer
    PUBLIC Threads::Threads
    PUBLIC malbolge_lib
)

target_compile_options(malbolge_address_sanitizer
    PRIVATE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
)

target_link_options(malbolge_address_sanitizer
    PRIVATE -fsanitize=address -fsanitize=undefined
)
