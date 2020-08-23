# Copyright Cam Mannett 2020
#
# See LICENSE file
#

if(NOT DEFINED ENV{EMSDK})
    message(FATAL_ERROR "EMSDK env var must be set")
endif()

add_executable(malbolge_wasm ${MAIN_SRCS})
add_dependencies(malbolge_wasm gen_version malbolge_lib)

target_compile_features(malbolge_wasm PUBLIC cxx_std_20)
set_target_properties(malbolge_wasm PROPERTIES CXX_EXTENSIONS OFF)

# The output files have .wasm and .js extensions, so there's no need for the
# _wasm suffix
set_target_properties(malbolge_wasm PROPERTIES OUTPUT_NAME malbolge)

# This is set by the Emscripten environment, but adding here helps the IDE
target_compile_definitions(malbolge_wasm
    PUBLIC EMSCRIPTEN
)

set(WASM_BUILD_OPTIONS
    "SHELL:-s ASYNCIFY"
    "SHELL:-s ASYNCIFY_IMPORTS=[\"malbolge_input\"]"
    "SHELL:-s ALLOW_MEMORY_GROWTH"
    "SHELL:-s PROXY_TO_PTHREAD"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-s USE_BOOST_HEADERS"
    "SHELL:-s USE_PTHREADS"
    "SHELL:-s PTHREAD_POOL_SIZE=\"1\"" # One more is added due to PROXY_TO_PTHREAD
    "SHELL:-s NO_INVOKE_RUN"
    "SHELL:-s ENVIRONMENT=\"web,worker\""
    "SHELL:-s MINIMAL_RUNTIME_STREAMING_WASM_INSTANTIATION"
)

# There seems to be a bug in either CMake or Emscripten that prevents the
# C++ standard flag from appearing in the flags when set using
# target_compile_features
target_compile_options(malbolge_wasm
    PRIVATE -std=c++20 -Werror -Wall -Wextra ${WASM_BUILD_OPTIONS}
)

target_link_options(malbolge_wasm
    PRIVATE ${WASM_BUILD_OPTIONS}
)

# Append the WASM build options to the library
target_compile_options(malbolge_lib
    PRIVATE -std=c++20 ${WASM_BUILD_OPTIONS}
)

target_link_options(malbolge_lib
    PRIVATE ${WASM_BUILD_OPTIONS}
)

target_include_directories(malbolge_wasm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC $ENV{EMSDK}/upstream/emscripten/system/include
)

target_link_libraries(malbolge_wasm
    PUBLIC Threads::Threads
    PUBLIC malbolge_lib
)
