# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge_test_coverage EXCLUDE_FROM_ALL ${HEADERS} ${SRCS} ${FOR_IDE})

target_compile_features(malbolge_test_coverage PUBLIC cxx_std_20)
set_target_properties(malbolge_test_coverage PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_test_coverage PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Werror -Wall -Wextra>
)

target_include_directories(malbolge_test_coverage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(malbolge_test_coverage
    PUBLIC Boost::unit_test_framework
    PUBLIC Threads::Threads
    PUBLIC gcov
)

target_compile_definitions(malbolge_test_coverage
    PUBLIC BOOST_TEST_DYN_LINK
)

target_compile_options(malbolge_test_coverage
    PRIVATE --coverage
)
