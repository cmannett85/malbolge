/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/utility/stream_helpers.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(stream_helpers_suite)

BOOST_AUTO_TEST_CASE(data_available)
{
    auto ss = std::stringstream{};
    BOOST_CHECK(!utility::data_available(ss));

    ss << "hello";
    BOOST_CHECK(utility::data_available(ss));

    auto sink = ""s;
    ss >> sink;
    BOOST_CHECK(!utility::data_available(ss));
    BOOST_CHECK_EQUAL(sink, "hello");

    ss.clear();
    ss << "goodbye";
    BOOST_CHECK(utility::data_available(ss));

    sink = ""s;
    ss >> sink;
    BOOST_CHECK(!utility::data_available(ss));
    BOOST_CHECK_EQUAL(sink, "goodbye");
}

BOOST_AUTO_TEST_SUITE_END()
