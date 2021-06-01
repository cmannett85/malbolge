/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/log.hpp"
#include "malbolge/utility/string_view_ops.hpp"

#include "test_helpers.hpp"

#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/find.hpp>

using namespace malbolge;
using namespace utility::string_view_ops;
using namespace std::string_literals;

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

BOOST_AUTO_TEST_CASE(set_log_stream)
{
    auto ss = std::stringstream{};
    log::set_log_stream(ss);

    auto f = [&](const auto& message, auto lvl) {
        log::print(lvl, message);
        const auto expected = "["s + to_string(lvl) + "]: " + message +
                              log::detail::colour_to_ansi();

        auto tmp = ""s;
        std::getline(ss, tmp);

        // Remove the timestamp
        const auto it = boost::algorithm::find_nth(tmp, "[", 1);
        auto tail = std::string_view{it.begin(), tmp.end()};

        BOOST_CHECK_EQUAL(tail, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"hello", log::INFO},
            std::tuple{"goodbye", log::ERROR},
        }
    );

    // Set the output stream back to the standard one!
    log::set_log_stream(std::clog);
}

BOOST_AUTO_TEST_SUITE_END()
