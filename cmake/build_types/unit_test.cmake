# Copyright Cam Mannett 2020
#
# See LICENSE file
#

set(Boost_USE_MULTITHREADED ON)
find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS
    unit_test_framework
)

set(TEST_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_helpers.hpp
)

set(TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm/container_ops_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm/remove_from_range_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c_interface_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu_instruction_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debugger_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ipow_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/tritset_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ternary_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/normalise_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/argument_parser_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/gate_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/raii_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/stream_lock_guard_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual_memory_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual_cpu_test.cpp
)

set(TEST_FOR_IDE
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/hello_world.mal
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/echo.mal
    ${CMAKE_CURRENT_SOURCE_DIR}/calculate_test_coverage.sh
)

add_executable(malbolge_test EXCLUDE_FROM_ALL ${TEST_HEADERS} ${TEST_SRCS} ${TEST_FOR_IDE})
add_dependencies(malbolge_test gen_version malbolge_lib)

target_compile_features(malbolge_test PUBLIC cxx_std_20)
set_target_properties(malbolge_test PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_test PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

target_include_directories(malbolge_test
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(malbolge_test
    PUBLIC Boost::unit_test_framework
    PUBLIC Threads::Threads
    PUBLIC malbolge_lib
)

target_compile_definitions(malbolge_test
    PUBLIC BOOST_TEST_DYN_LINK
)

message(STATUS "Copying over example programs")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/programs
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME malbolge_test COMMAND malbolge_test -l test_suite)
