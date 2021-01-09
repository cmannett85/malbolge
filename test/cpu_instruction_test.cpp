/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/cpu_instruction.hpp"

#include "test_helpers.hpp"

#include <limits>

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(cpu_instruction_suite)

BOOST_AUTO_TEST_CASE(type)
{
    auto f = [](auto instruction, auto expected) {
        BOOST_CHECK_EQUAL(instruction, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{cpu_instruction::set_data_ptr,   'j'},
            std::tuple{cpu_instruction::set_code_ptr,   'i'},
            std::tuple{cpu_instruction::rotate,         '*'},
            std::tuple{cpu_instruction::op,             'p'},
            std::tuple{cpu_instruction::read,           '/'},
            std::tuple{cpu_instruction::write,          '<'},
            std::tuple{cpu_instruction::stop,           'v'},
            std::tuple{cpu_instruction::nop,            'o'},
        }
    );
}

BOOST_AUTO_TEST_CASE(is_cpu_instruction_test)
{
    for (auto i = std::numeric_limits<char>::min();
         i < std::numeric_limits<char>::max(); ++i) {
        const auto expected = std::ranges::find(cpu_instruction::all, i) !=
                              cpu_instruction::all.end();
        const auto result = is_cpu_instruction(i);
        BOOST_CHECK_EQUAL(result, expected);
    }
}

BOOST_AUTO_TEST_CASE(graphical_ascii)
{
    for (auto i = std::numeric_limits<char>::min();
         i < std::numeric_limits<char>::max(); ++i) {
        const auto expected = i >= 33 && i <= 126;
        BOOST_CHECK_EQUAL(is_graphical_ascii(i), expected);
    }
}

BOOST_AUTO_TEST_CASE(cipher_pre)
{
    constexpr auto pre_cipher = R"(+b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XTcA"lI)"
                                R"(.v%{gJh4G\-=O@5`_3i<?Z';FNQuY]szf$!BS/|t:Pn6^Ha)";
    static_assert(std::string_view{pre_cipher}.size() == cipher::size,
                  "Pre cipher size has changed");

    for (auto i = 0u; i < cipher::size + 1; ++i) {
        const auto result = cipher::pre(i);
        if (i < cipher::size) {
            BOOST_REQUIRE(result);
            BOOST_CHECK_EQUAL(*result, pre_cipher[i]);
        } else {
            BOOST_CHECK(!result);
        }
    }
}

BOOST_AUTO_TEST_CASE(cipher_post)
{
    constexpr auto post_cipher = R"(5z]&gqtyfr$(we4{WP)H-Zn,[%\3dL+Q;>U!pJS72FhOA1C)"
                                 R"(B6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G"i@)";
    static_assert(std::string_view{post_cipher}.size() == cipher::size,
                  "Post  cipher size has changed");

    for (auto i = 0u; i < cipher::size + 1; ++i) {
        const auto result = cipher::post(i);
        if (i < cipher::size) {
            BOOST_REQUIRE(result);
            BOOST_CHECK_EQUAL(*result, post_cipher[i]);
        } else {
            BOOST_CHECK(!result);
        }
    }
}

BOOST_AUTO_TEST_CASE(pre_cipher_instruction_test)
{
    auto f = [](auto input, auto index, auto expected) {
        const auto result = pre_cipher_instruction(input, index);
        BOOST_REQUIRE_EQUAL(!result, !expected);
        if (result) {
            BOOST_CHECK_EQUAL(*result, *expected);
        };
    };

    test::data_set(
        f,
        {
            std::tuple{'a',   4, std::optional<char>{'Z'}},
            std::tuple{'a',  10, std::optional<char>{'u'}},
            std::tuple{'H', 120, std::optional<char>{'i'}},
            std::tuple{'\n',  0, std::optional<char>{}},
        }
    );
}

BOOST_AUTO_TEST_CASE(post_cipher_instruction_test)
{
    auto f = [](auto input, auto expected) {
        const auto result = post_cipher_instruction(input);
        BOOST_REQUIRE_EQUAL(!result, !expected);
        if (result) {
            BOOST_CHECK_EQUAL(*result, *expected);
        };
    };

    test::data_set(
        f,
        {
            std::tuple{'a',  std::optional<char>{'.'}},
            std::tuple{'b',  std::optional<char>{'T'}},
            std::tuple{'!',  std::optional<char>{'5'}},
            std::tuple{'\n', std::optional<char>{}},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
