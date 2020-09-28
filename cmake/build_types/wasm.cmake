# Copyright Cam Mannett 2020
#
# See LICENSE file
#

if(NOT DEFINED ENV{EMSDK})
    message(FATAL_ERROR "EMSDK env var must be set")
endif()

set(WASM_BUILD_OPTIONS
    "-Wno-pthreads-mem-growth"
    "SHELL:-s EXPORTED_FUNCTIONS=[\"_malbolge_log_level\",\"_malbolge_set_log_level\",\"_malbolge_version\",\"_malbolge_load_program\",\"_malbolge_load_normalised_program\",\"_malbolge_free_virtual_memory\",\"_malbolge_vcpu_run_wasm\",\"_malbolge_vcpu_stop\",\"_malbolge_vcpu_input\",\"_malbolge_is_likely_normalised_source\",\"_malbolge_normalise_source\",\"_malbolge_denormalise_source\"]"
    "SHELL:-s ALLOW_MEMORY_GROWTH"
    "SHELL:-s ALLOW_TABLE_GROWTH"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-s USE_BOOST_HEADERS"
    "SHELL:-s USE_PTHREADS"
    "SHELL:-s PTHREAD_POOL_SIZE=\"1\"" # One more is added due to PROXY_TO_PTHREAD
    "SHELL:-s ENVIRONMENT=\"web,worker\""
    "SHELL:-s MINIMAL_RUNTIME_STREAMING_WASM_INSTANTIATION"
)

# Append the WASM build options to the library
target_compile_options(malbolge_lib
    PRIVATE -std=c++20 ${WASM_BUILD_OPTIONS}
)

target_link_options(malbolge_lib
    PRIVATE ${WASM_BUILD_OPTIONS}
)

set(WASM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/malbolge/c_interface_wasm.hpp
)

set(WASM_IDE
    ${CMAKE_CURRENT_SOURCE_DIR}/playground/index.html
)

# Because Emscripten needs em++ to produce a WASM file, we have to 'wrap' the
# library in an executable target
add_executable(malbolge_wasm ${WASM_HEADERS} ${WASM_IDE})
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

# There seems to be a bug in either CMake or Emscripten that prevents the
# C++ standard flag from appearing in the flags when set using
# target_compile_features
target_compile_options(malbolge_wasm
    PRIVATE -std=c++20 -Werror -Wall -Wextra ${WASM_BUILD_OPTIONS}
)

target_link_options(malbolge_wasm
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
