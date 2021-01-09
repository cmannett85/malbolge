/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/unescaper.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(utility_suite)

BOOST_AUTO_TEST_CASE(unescape_ascii_test)
{
    auto f = [](std::string_view input, auto expected, auto exception) {
        try {
            const auto result = utility::unescape_ascii(input);
            BOOST_CHECK(!exception);
            BOOST_CHECK_EQUAL(result, expected);
        } catch (std::exception&) {
            BOOST_CHECK(exception);
        }
    };

    test::data_set(
        f,
        {
            // Control characters
            std::tuple{R"(\a\b\t\n\v\f\r)",     "\a\b\t\n\v\f\r"s,  false},
            std::tuple{R"(\c)",                 ""s,                true},
            // Punctuation characters
            std::tuple{R"(\"\'\?\\)",           "\"\'\?\\"s,        false},
            // Octal
            std::tuple{R"(\0)",                 "\0"s,              false},
            std::tuple{R"(\01)",                "\01"s,             false},
            std::tuple{R"(\012)",               "\n"s,              false},
            std::tuple{R"(\12)",                "\n"s,              false},
            std::tuple{R"(\0123)",              "\n3"s,             false},
            // Hex
            std::tuple{R"(\xA)",                "\n"s,              false},
            std::tuple{R"(\xa)",                "\n"s,              false},
            std::tuple{R"(\x28)",               "("s,               false},
            std::tuple{R"(\x4C)",               "L"s,               false},
            std::tuple{R"(\x4Cf)",              ""s,                true},
            std::tuple{R"(\x)",                 ""s,                true},
            // Embedded into strings
            std::tuple{R"(\"hello wor\154d\")", "\"hello world\""s, false},
            std::tuple{R"(\"hello worl\x64\")", "\"hello world\""s, false},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
