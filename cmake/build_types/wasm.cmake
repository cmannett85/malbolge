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

target_compile_options(malbolge_wasm
    PRIVATE --std=c++20 -Werror -Wall -Wextra -s USE_BOOST_HEADERS=1 #-s USE_PTHREADS=1
)

# target_link_options(malbolge_wasm
#     PRIVATE -s USE_BOOST_HEADERS=1 -s USE_PTHREADS=1
# )

target_include_directories(malbolge_wasm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(malbolge_wasm
    PUBLIC Threads::Threads
)
