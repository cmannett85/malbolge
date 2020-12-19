/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/from_chars.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace malbolge::literals;
using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(from_chars_suite)

BOOST_AUTO_TEST_CASE(from_chars_with_base_test)
{
    auto f = [](auto input, auto base, auto expected, auto exception_thrown) {
        using T = std::decay_t<decltype(expected)>;

        try {
            const auto result = utility::from_chars_with_base<T>(input, base);
            BOOST_CHECK(!exception_thrown);
            BOOST_CHECK_EQUAL(result, expected);
        } catch (std::exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            BOOST_CHECK(exception_thrown);
        }
    };

    test::data_set(
        f,
        std::tuple{
            std::tuple{"0"sv,       10, std::uint8_t{0},        false},
            std::tuple{"42"sv,      10, std::uint32_t{42},      false},
            std::tuple{"+42"sv,     10, std::uint32_t{42},      false},
            std::tuple{"-42"sv,     10, std::int32_t{-42},      false},
            std::tuple{"1000"sv,    16, std::uint32_t{4096},    false},
            std::tuple{"1000"sv,    16, std::uint32_t{4096},    false},
            std::tuple{"Aa"sv,      16, std::uint32_t{170},     false},
            std::tuple{"245"sv,      8, std::uint32_t{165},     false},
            std::tuple{""sv,        10, 0,                      true},
            std::tuple{"hello"sv,   10, 0,                      true},
            std::tuple{"1000"sv,    10, std::uint8_t{0},        true},
            std::tuple{"Aa"sv,      10, std::uint32_t{0},       true},
        }
    );
}

BOOST_AUTO_TEST_CASE(from_chars_test)
{
    auto f = [](auto input, auto expected, auto exception_thrown) {
        using T = std::decay_t<decltype(expected)>;

        try {
            const auto result = utility::from_chars<T>(input);
            BOOST_CHECK(!exception_thrown);
            BOOST_CHECK_EQUAL(result, expected);
        } catch (std::exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            BOOST_CHECK(exception_thrown);
        }
    };

    test::data_set(
        f,
        std::tuple{
            std::tuple{"0"sv,           std::uint8_t{0},            false},
            std::tuple{"42"sv,          std::uint32_t{42},          false},
            std::tuple{"+42"sv,         std::uint32_t{42},          false},
            std::tuple{"-42"sv,         std::int32_t{-42},          false},
            std::tuple{"0x1000"sv,      std::uint32_t{4096},        false},
            std::tuple{"0X1000"sv,      std::uint32_t{4096},        false},
            std::tuple{"0245"sv,        std::uint32_t{165},         false},
            std::tuple{""sv,            0,                          true},
            std::tuple{"hello"sv,       0,                          true},
            std::tuple{"1000"sv,        std::uint8_t{0},            true},

            std::tuple{"4.2"sv,         4.2,                        false},
            std::tuple{"4.2e2"sv,       420.0,                      false},
            std::tuple{"4.2E2"sv,       420.0,                      false},
            std::tuple{"+4.2e2"sv,      420.0,                      false},
            std::tuple{"-4.2e2"sv,      -420.0,                     false},
            std::tuple{""sv,            0.0,                        true},
            std::tuple{"hello"sv,       0.0,                        true},
            std::tuple{"1.0e1000"sv,    0.0,                        true},

            std::tuple{"4.2"sv,         4.2f,                       false},
            std::tuple{"4.2e2"sv,       420.0f,                     false},
            std::tuple{"4.2E2"sv,       420.0f,                     false},
            std::tuple{"+4.2e2"sv,      420.0f,                     false},
            std::tuple{"-4.2e2"sv,      -420.0f,                    false},
            std::tuple{""sv,            0.0f,                       true},
            std::tuple{"hello"sv,       0.0f,                       true},
            std::tuple{"1.0e1000"sv,    0.0f,                       true},

            std::tuple{"t200"sv,        math::ternary{200_trit},    false},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
