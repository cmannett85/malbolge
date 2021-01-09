/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"
#include "malbolge/utility/raii.hpp"

#include "test_helpers.hpp"

#include <fstream>

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(loader_suite)

BOOST_AUTO_TEST_CASE(load_normalised_mode_streaming_operator)
{
    auto f = [](auto mode, auto expected) {
        auto ss = std::stringstream{};
        ss << mode;
        BOOST_CHECK_EQUAL(ss.str(), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{load_normalised_mode::AUTO, "AUTO"},
            std::tuple{load_normalised_mode::ON, "ON"},
            std::tuple{load_normalised_mode::OFF, "OFF"},
            std::tuple{static_cast<load_normalised_mode>(42), "Unknown"},
        }
    );
}

BOOST_AUTO_TEST_CASE(load_test)
{
    using pdata_t = std::tuple<std::vector<char>,
                               std::vector<char>,
                               bool,
                               optional_source_location>;
    using stream_iterator = std::ostreambuf_iterator<char>;

    const auto tmp_path = std::filesystem::temp_directory_path() /
                          "loader_range.mal";

    auto f = [&](auto program_data, auto parse_data, auto throws, auto loc) {
        BOOST_TEST_MESSAGE("In memory");
        try {
            auto input_data = program_data;
            auto vmem = load(input_data, load_normalised_mode::OFF);
            if (throws) {
                BOOST_FAIL("Should have thrown");
            }

            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        } catch (parse_exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            if (!throws) {
                BOOST_FAIL("Should not have thrown");
            }
            BOOST_CHECK_EQUAL(e.location(), loc);
        }

        BOOST_TEST_MESSAGE("From disk");
        try {
            {
                auto input_data = program_data;
                auto stream = std::ofstream{};
                stream.exceptions(std::ios::badbit | std::ios::failbit);
                stream.open(tmp_path, std::ios::trunc | std::ios::binary);

                std::copy(input_data.begin(),
                          input_data.end(),
                          stream_iterator{stream});
            }

            auto vmem = load(tmp_path, load_normalised_mode::OFF);
            if (loc) {
                BOOST_FAIL("Should have thrown");
            }

            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        } catch (parse_exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            if (!throws) {
                BOOST_FAIL("Should not have thrown");
            }
            BOOST_CHECK_EQUAL(e.location(), loc);
        }

        BOOST_TEST_MESSAGE("From cin");
        try {
            // Temporarily take control of std::cin
            auto orig_rdbuf = std::cin.rdbuf();
            auto raii = utility::raii{[orig_rdbuf]() {
                std::cin.rdbuf(orig_rdbuf);
            }};

            auto ss = std::stringstream{};
            for (auto c : program_data) {
                ss << c;
            }

            std::cin.rdbuf(ss.rdbuf());

            auto vmem = load_from_cin(load_normalised_mode::OFF);
            if (loc) {
                BOOST_FAIL("Should have thrown");
            }

            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        } catch (parse_exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            if (!throws) {
                BOOST_FAIL("Should not have thrown");
            }
            BOOST_CHECK_EQUAL(e.location(), loc);
        }
    };

    test::data_set(
        f,
        {
            pdata_t{{},
                    {},
                    true,
                    optional_source_location{}},
            pdata_t{{40},
                    {},
                    true,
                    optional_source_location{}},
            pdata_t{{40, 39},
                    {40, 39},
                    false,
                    optional_source_location{}},
            pdata_t{{' ', '\0'},
                    {},
                    true,
                    optional_source_location{source_location{1, 2}}},
            pdata_t{{' ', '\t', '\n'},
                    {},
                    true,
                    optional_source_location{}},
            pdata_t{{' ', '\t', '\n', 40},
                    {40},
                    true,
                    optional_source_location{}},
            pdata_t{{' ', '\t', '\n', 40, 39},
                    {40, 39},
                    false,
                    optional_source_location{}},
        }
    );
}

BOOST_AUTO_TEST_CASE(bad_file_path)
{
    try {
        auto vmem = load(std::filesystem::path{"/tmp/nonsense"});
        BOOST_CHECK_MESSAGE(false, "Should have thrown");
    } catch (std::exception& e) {
        BOOST_TEST_MESSAGE(e.what());
    }
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
