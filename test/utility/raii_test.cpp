/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/raii.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(utility_suite)

BOOST_AUTO_TEST_SUITE(raii_suite)

BOOST_AUTO_TEST_CASE(constructor_test)
{
    auto fired = 0;
    {
        auto raii = utility::raii{[&]() { fired = 1; }};
    }
    BOOST_CHECK_EQUAL(fired, 1);

    fired = 0;
    {
        auto raii1 = utility::raii{[&]() { fired = 1; }};
        auto raii2 = utility::raii{[&]() { fired = 2; }};
        raii1 = raii2;
        BOOST_CHECK_EQUAL(fired, 1);
    }
    BOOST_CHECK_EQUAL(fired, 2);

    fired = 0;
    {
        auto raii1 = utility::raii{[&]() { fired = 1; }};
        auto raii2 = utility::raii{[&]() { fired = 2; }};
        raii1 = std::move(raii2);
        BOOST_CHECK_EQUAL(fired, 1);
    }
    BOOST_CHECK_EQUAL(fired, 2);
}

BOOST_AUTO_TEST_CASE(reset_valid_test)
{
    auto fired = 0;
    {
        auto raii = utility::raii{[&]() { fired = 1; }};
        raii.reset([&]() { fired = 2; });
        BOOST_CHECK_EQUAL(fired, 0);
    }
    BOOST_CHECK_EQUAL(fired, 2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
