/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/algorithm/container_ops.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(container_ops_suite)

BOOST_AUTO_TEST_CASE(any_of_test)
{
    auto f = [&](auto&& c, auto&& value, auto&& expected) {
        BOOST_CHECK_EQUAL(algorithm::any_of(c, value), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{std::vector{0, 1, 2, 3, 4}, 0, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, 4, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, 2, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, 8, false},
            std::tuple{std::vector<int>{}, 8, false},
        }
    );
}

BOOST_AUTO_TEST_CASE(any_of_container_test)
{
    auto f = [&](auto&& c, auto&& value, auto&& expected) {
        BOOST_CHECK_EQUAL(algorithm::any_of_container(c, value), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{std::vector{0, 1, 2, 3, 4}, std::vector{0}, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, std::vector{8, 0}, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, std::vector{0, 8}, true},
            std::tuple{std::vector{0, 1, 2, 3, 4}, std::vector{9, 8}, false},
            std::tuple{std::vector{0, 1, 2, 3, 4}, std::vector<int>{}, false},
            std::tuple{std::vector<int>{}, std::vector{0}, false},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
