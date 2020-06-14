# Copyright Cam Mannett 2020
#
# See LICENSE file
#

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.67 REQUIRED COMPONENTS
    unit_test_framework
)

list(APPEND HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_helpers.hpp
)

list(APPEND SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ipow_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/tritset_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/ternary_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual_memory_test.cpp
)

list(APPEND FOR_IDE

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
    PUBLIC Boost::unit_test_framework
)

target_compile_definitions(malbolge_test
    PUBLIC BOOST_TEST_DYN_LINK
    PUBLIC TEST_BUILD
)

add_test(NAME malbolge_test COMMAND malbolge_test -l test_suite)
