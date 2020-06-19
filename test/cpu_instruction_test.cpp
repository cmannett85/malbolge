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
            std::tuple{cpu_instruction::read,           '<'},
            std::tuple{cpu_instruction::write,          '/'},
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

BOOST_AUTO_TEST_SUITE_END()
