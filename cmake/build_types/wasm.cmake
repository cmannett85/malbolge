# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge_wasm ${HEADERS} ${SRCS} ${MAIN_SRCS} ${FOR_IDE} ${VERSION_FILE})
add_dependencies(malbolge_wasm gen_version)

target_compile_features(malbolge_wasm PUBLIC cxx_std_20)
set_target_properties(malbolge_wasm PROPERTIES CXX_EXTENSIONS OFF)

# This is set by the Emscripten environment, but adding here helps the IDE
target_compile_definitions(malbolge_wasm
    PUBLIC EMSCRIPTEN
)

set(WASM_BUILD_OPTIONS
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-s USE_BOOST_HEADERS"
    "SHELL:-s USE_PTHREADS"
    "SHELL:-s PTHREAD_POOL_SIZE=\"2\""
    "SHELL:-s NO_INVOKE_RUN"
    "SHELL:-s ENVIRONMENT=\"web,worker\""
    "SHELL:-s MINIMAL_RUNTIME_STREAMING_WASM_INSTANTIATION"
)

# There seems to be a bug in either CMake or Emscripten that prevents the
# C++ standard flag from appearing in the flags when set using
# target_compile_features
target_compile_options(malbolge_wasm
    PRIVATE --std=c++20 -Werror -Wall -Wextra ${WASM_BUILD_OPTIONS}
)

target_link_options(malbolge_wasm
    PRIVATE ${WASM_BUILD_OPTIONS}
)

target_include_directories(malbolge_wasm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_wasm
    PUBLIC Threads::Threads
)
