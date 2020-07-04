# Copyright Cam Mannett 2020
#
# See LICENSE file
#

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.73 REQUIRED COMPONENTS
    log
    unit_test_framework
)
find_package(Threads REQUIRED)

list(APPEND HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_helpers.hpp
)

list(APPEND SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ipow_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/tritset_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ternary_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual_memory_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu_instruction_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/algorithm/remove_from_range_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual_cpu_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/raii_test.cpp
)

list(APPEND FOR_IDE
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/hello_world.mal
    ${CMAKE_CURRENT_SOURCE_DIR}/programs/echo.mal
)

add_executable(malbolge_test ${HEADERS} ${SRCS} ${FOR_IDE})

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
    PUBLIC Boost::log
    PUBLIC Boost::unit_test_framework
    PUBLIC Threads::Threads
)

target_compile_definitions(malbolge_test
    PUBLIC BOOST_TEST_DYN_LINK
    PUBLIC BOOST_LOG_DYN_LINK
    PUBLIC TEST_BUILD
)

message("Copying over example programs")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/programs
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME malbolge_test COMMAND malbolge_test -l test_suite)
