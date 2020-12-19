/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/string_constant.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(string_constant_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    const auto hello = MAL_STR(hello){};
    BOOST_CHECK_EQUAL(hello.size(), 5u);
    BOOST_CHECK_EQUAL(hello.value(), "hello"sv);
    BOOST_CHECK_EQUAL(hello[0], 'h');
    BOOST_CHECK_EQUAL(hello[3], 'l');

    try {
        BOOST_CHECK_EQUAL(hello.at(6), '\n');
        BOOST_FAIL("Should have thrown");
    } catch (std::out_of_range& e) {}
}

BOOST_AUTO_TEST_SUITE_END()
