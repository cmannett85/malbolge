# Copyright Cam Mannett 2020
#
# See LICENSE file
#

add_executable(malbolge_test_coverage EXCLUDE_FROM_ALL ${TEST_HEADERS} ${TEST_SRCS} ${TEST_FOR_IDE})
add_dependencies(malbolge_test_coverage gen_version malbolge_lib_coverage)

target_compile_features(malbolge_test_coverage PUBLIC cxx_std_20)
set_target_properties(malbolge_test_coverage PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(malbolge_test_coverage
    PRIVATE -Werror -Wall -Wextra
    PRIVATE --coverage
)

target_include_directories(malbolge_test_coverage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(malbolge_test_coverage
    PUBLIC Boost::unit_test_framework
    PUBLIC Threads::Threads
    PUBLIC malbolge_lib_coverage
)

target_compile_definitions(malbolge_test_coverage
    PUBLIC BOOST_TEST_DYN_LINK
)

target_link_options(malbolge_test_coverage
    PRIVATE --coverage
)
