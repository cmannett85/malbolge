/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/log.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(log_suite)

BOOST_AUTO_TEST_CASE(level_string_conversion)
{
    auto f = [](auto lvl, auto expected) {
        const auto result = log::to_string(lvl);
        BOOST_CHECK_EQUAL(result, expected);

        auto ss = std::stringstream{};
        ss << lvl;
        BOOST_CHECK_EQUAL(ss.str(), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{log::VERBOSE_DEBUG,  "VERBOSE DEBUG"},
            std::tuple{log::DEBUG,          "DEBUG"},
            std::tuple{log::INFO,           "INFO"},
            std::tuple{log::ERROR,          "ERROR"},
            std::tuple{log::NUM_LOG_LEVELS, "Unknown"},
        }
    );
}

BOOST_AUTO_TEST_CASE(colour_to_ansi)
{
    auto f = [](auto colour, auto expected) {
        const auto result = log::detail::colour_to_ansi(colour);
        BOOST_CHECK_EQUAL(result, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{log::colour::DEFAULT,        "\x1B[0m"},
            std::tuple{log::colour::RED,            "\x1B[31m"},
            std::tuple{log::colour::GREEN,          "\x1B[32m"},
            std::tuple{log::colour::YELLOW,         "\x1B[33m"},
            std::tuple{log::colour::BLUE,           "\x1B[34m"},
            std::tuple{log::colour::NUM_COLOURS,    "\x1B[0m"},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
