/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

#include <fstream>

using namespace malbolge;

namespace
{
using pdata_t = std::tuple<std::vector<char>, std::vector<char>, bool>;
using stream_iterator = std::ostreambuf_iterator<char>;

const auto tmp_path = std::filesystem::temp_directory_path() /
                      "loader_range.mal";
}

BOOST_AUTO_TEST_SUITE(loader_suite)

BOOST_AUTO_TEST_CASE(load_test)
{
    auto f = [](auto program_data, auto parse_data, auto throws) {
        try {
            auto vmem = load(program_data);
            if (throws) {
                BOOST_FAIL("Should have thrown");
            }

            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        } catch (std::exception& e) {
            if (!throws) {
                BOOST_FAIL("Should not have thrown: " << e.what());
            }
        }

        try {
            {
                auto stream = std::ofstream{};
                stream.exceptions(std::ios::badbit | std::ios::failbit);
                stream.open(tmp_path, std::ios::trunc | std::ios::binary);

                std::copy(program_data.begin(),
                          program_data.end(),
                          stream_iterator{stream});
            }

            auto vmem = load(tmp_path);
            if (throws) {
                BOOST_FAIL("Should have thrown");
            }

            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        } catch (std::exception& e) {
            if (!throws) {
                BOOST_FAIL("Should not have thrown: " << e.what());
            }
        }
    };

    test::data_set(
        f,
        {
            pdata_t{{},
                    {},
                    true},
            pdata_t{{cpu_instruction::set_code_ptr},
                    {},
                    true},
            pdata_t{{cpu_instruction::set_code_ptr,
                     cpu_instruction::set_data_ptr},
                    {cpu_instruction::set_code_ptr,
                     cpu_instruction::set_data_ptr},
                    false},
            pdata_t{{' ', '\0'},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n'},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n',
                     cpu_instruction::set_code_ptr},
                    {cpu_instruction::set_code_ptr},
                    true},
            pdata_t{{' ', '\t', '\n',
                     cpu_instruction::set_code_ptr,
                     cpu_instruction::set_data_ptr},
                    {cpu_instruction::set_code_ptr,
                     cpu_instruction::set_data_ptr},
                    false},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
