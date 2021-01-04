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
        auto orig_program_data = program_data;

        BOOST_TEST_MESSAGE("In memory");
        try {
            auto vmem = load(program_data, load_normalised_mode::OFF);
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

        BOOST_TEST_MESSAGE("From disk");
        try {
            {
                auto stream = std::ofstream{};
                stream.exceptions(std::ios::badbit | std::ios::failbit);
                stream.open(tmp_path, std::ios::trunc | std::ios::binary);

                std::copy(orig_program_data.begin(),
                          orig_program_data.end(),
                          stream_iterator{stream});
            }

            auto vmem = load(tmp_path, load_normalised_mode::OFF);
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
            pdata_t{{40},
                    {},
                    true},
            pdata_t{{40, 39},
                    {40, 39},
                    false},
            pdata_t{{' ', '\0'},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n'},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n', 40},
                    {40},
                    true},
            pdata_t{{' ', '\t', '\n', 40, 39},
                    {40, 39},
                    false},
        }
    );
}

BOOST_AUTO_TEST_CASE(normalisation_test)
{
    log::set_log_level(log::DEBUG);

    auto f = [](auto path, auto mode, auto fail) {
        try {
            auto vmem = load(std::filesystem::path{path}, mode);
            BOOST_CHECK_MESSAGE(!fail, "Should have thrown");
        } catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(fail, "Should not have thrown");
            BOOST_TEST_MESSAGE(e.what());
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"programs/hello_world.mal",
                       load_normalised_mode::AUTO,
                       false},
            std::tuple{"programs/hello_world_normalised.mal",
                       load_normalised_mode::AUTO,
                       false},
            std::tuple{"programs/hello_world.mal",
                       load_normalised_mode::ON,
                       true},
            std::tuple{"programs/hello_world_normalised.mal",
                       load_normalised_mode::ON,
                       false},
            std::tuple{"programs/hello_world.mal",
                       load_normalised_mode::OFF,
                       false},
            std::tuple{"programs/hello_world_normalised.mal",
                       load_normalised_mode::OFF,
                       true},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
