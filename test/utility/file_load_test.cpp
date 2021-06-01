/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/file_load.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

namespace
{
const auto expected_raw = std::string{R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>
)"};
}

BOOST_AUTO_TEST_SUITE(file_load_suite)

BOOST_AUTO_TEST_CASE(file_alloc)
{
    auto f = [](auto path, auto fail) {
        const auto expected = std::vector<char>(expected_raw.begin(),
                                                expected_raw.end());

        try {
            auto data = utility::file_load<std::vector<char>>(path);
            BOOST_CHECK_MESSAGE(!fail, "Should have failed");
            if (!fail) {
                BOOST_REQUIRE_EQUAL(expected.size(), data.size());
                for (auto i = 0u; i < expected.size(); ++i) {
                    BOOST_CHECK_EQUAL(data[i], expected[i]);
                }
            }
        } catch (parse_exception& e) {
            BOOST_CHECK_MESSAGE(fail, "Should not have failed: " << e.what());
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"programs/hello_world.mal", false},
            std::tuple{"programs/wrong.mal", true},
        }
    );
}

BOOST_AUTO_TEST_CASE(file_alloc_string)
{
    auto f = [](auto path, auto fail) {
        const auto expected = expected_raw;

        try {
            auto data = utility::file_load<std::string>(path);
            BOOST_CHECK_MESSAGE(!fail, "Should have failed");
            if (!fail) {
                BOOST_REQUIRE_EQUAL(expected.size(), data.size());
                for (auto i = 0u; i < expected.size(); ++i) {
                    BOOST_CHECK_EQUAL(data[i], expected[i]);
                }
            }
        } catch (parse_exception& e) {
            BOOST_CHECK_MESSAGE(fail, "Should not have failed: " << e.what());
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"programs/hello_world.mal", false},
            std::tuple{"programs/wrong.mal", true},
        }
    );
}

BOOST_AUTO_TEST_CASE(file_no_alloc)
{
    auto f = [](auto path, auto fail) {
        const auto expected = std::vector<char>(expected_raw.begin(),
                                                expected_raw.end());

        try {
            auto data = utility::file_load<std::array<char, 132>>(path);
            BOOST_CHECK_MESSAGE(!fail, "Should have failed");
            if (!fail) {
                BOOST_REQUIRE_EQUAL(expected.size(), data.size());
                for (auto i = 0u; i < expected.size(); ++i) {
                    BOOST_CHECK_EQUAL(data[i], expected[i]);
                }
            }
        } catch (parse_exception& e) {
            BOOST_CHECK_MESSAGE(fail, "Should not have failed: " << e.what());
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"programs/hello_world.mal", false},
            std::tuple{"programs/wrong.mal", true},
        }
    );

    // Too small container
    try {
        auto data = utility::file_load<std::array<char, 131>>("programs/hello_world.mal");
        boost::ignore_unused(data);
        BOOST_FAIL("Should have thrown");
    } catch (parse_exception&) {}
}

BOOST_AUTO_TEST_SUITE_END()
