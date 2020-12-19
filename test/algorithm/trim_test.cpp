/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/algorithm/trim.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;
using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(trim_suite)

BOOST_AUTO_TEST_CASE(left_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto&& input, auto&& expected) {
        auto begin = input.begin();
        auto end = input.end();
        algorithm::trim_left(begin, end, p);

        const auto result = std::decay_t<decltype(input)>(begin, end);
        BOOST_CHECK_EQUAL(result, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"s,   "hello"s},
            std::tuple{" hello"s,  "hello"s},
            std::tuple{"  hello"s, "hello"s},
            std::tuple{" hello "s, "hello "s},
        }
    );
}

BOOST_AUTO_TEST_CASE(sv_left_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto input, auto&& expected) {
        algorithm::trim_left(input, p);
        BOOST_CHECK_EQUAL(input, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"sv,   "hello"sv},
            std::tuple{" hello"sv,  "hello"sv},
            std::tuple{"  hello"sv, "hello"sv},
            std::tuple{" hello "sv, "hello "sv},
        }
    );
}

BOOST_AUTO_TEST_CASE(right_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto&& input, auto&& expected) {
        auto begin = input.begin();
        auto end = input.end();
        algorithm::trim_right(begin, end, p);

        const auto result = std::decay_t<decltype(input)>(begin, end);
        BOOST_CHECK_EQUAL(result, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"s,   "hello"s},
            std::tuple{"hello "s,  "hello"s},
            std::tuple{"hello  "s, "hello"s},
            std::tuple{" hello "s, " hello"s},
        }
    );
}

BOOST_AUTO_TEST_CASE(sv_right_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto input, auto&& expected) {
        algorithm::trim_right(input, p);
        BOOST_CHECK_EQUAL(input, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"sv,   "hello"sv},
            std::tuple{"hello "sv,  "hello"sv},
            std::tuple{"hello  "sv, "hello"sv},
            std::tuple{" hello "sv, " hello"sv},
        }
    );
}

BOOST_AUTO_TEST_CASE(both_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto&& input, auto&& expected) {
        auto begin = input.begin();
        auto end = input.end();
        algorithm::trim(begin, end, p);

        const auto result = std::decay_t<decltype(input)>(begin, end);
        BOOST_CHECK_EQUAL(result, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"s,   "hello"s},
            std::tuple{" hello"s,  "hello"s},
            std::tuple{"  hello"s, "hello"s},
            std::tuple{"hello"s,   "hello"s},
            std::tuple{"hello "s,  "hello"s},
            std::tuple{"hello  "s, "hello"s},
            std::tuple{" hello "s, "hello"s},
        }
    );
}

BOOST_AUTO_TEST_CASE(sv_both_test)
{
    auto p = [](auto c) { return std::isspace(c); };

    auto f = [&](auto input, auto&& expected) {
        algorithm::trim(input, p);
        BOOST_CHECK_EQUAL(input, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello"sv,   "hello"sv},
            std::tuple{" hello"sv,  "hello"sv},
            std::tuple{"  hello"sv, "hello"sv},
            std::tuple{"hello"sv,   "hello"sv},
            std::tuple{"hello "sv,  "hello"sv},
            std::tuple{"hello  "sv, "hello"sv},
            std::tuple{" hello "sv, "hello"sv},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
