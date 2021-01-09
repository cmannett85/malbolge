/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/normalise.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(normalise_suite)

BOOST_AUTO_TEST_CASE(normalise_valid_test)
{
    auto f = [](auto&& source, auto&& expected) {
        BOOST_REQUIRE_EQUAL(source.size(), expected.size());

        // Iterator version
        {
            auto input = source;
            auto it = normalise_source(input.begin(), input.end());

            input.erase(it, input.end());
            BOOST_CHECK_EQUAL(input, expected);
        }

        // Range version
        {
            auto input = source;
            auto it = normalise_source(input);

            input.erase(it, input.end());
            BOOST_CHECK_EQUAL(input, expected);
        }

        // Resize version
        {
            auto input = source;
            normalise_source_resize(input);
            BOOST_CHECK_EQUAL(input, expected);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{R"_((=BA#9"=<;:3y7x54-21q/p-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       "jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s},
            std::tuple{R"_(('&%#^"!~}{XE)_"s,
                       "jjjj*<jjjj*<v"s},
            std::tuple{R"_(('&%$#"!~}|{zyxwvutsrqpnKmlkjihgfedcba`_^]\[ZYXWVT1|)_"s,
                       "jjjjjjjjjjjjjjjjjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s},
            std::tuple{""s,
                       ""s},
        }
    );
}

BOOST_AUTO_TEST_CASE(normalise_invalid_test)
{
    auto f = [](auto&& source, auto&& source_loc) {
        // Iterator version
        try {
            auto input = source;
            normalise_source(input.begin(), input.end());
            BOOST_FAIL("Should have thrown");
        } catch (parse_exception& e) {
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), source_loc);
        }

        // Range version
        try {
            auto input = source;
            normalise_source(input);
            BOOST_FAIL("Should have thrown");
        } catch (parse_exception& e) {
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), source_loc);
        }

        // Resize version
        try {
            auto input = source;
            normalise_source_resize(input);
            BOOST_FAIL("Should have thrown");
        } catch (parse_exception& e) {
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), source_loc);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{R"_((=BA#9"=<;:3y7x54-21q/-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       source_location{1, 23}},
            std::tuple{R"_(('&%#^"!~}{bE)_"s,
                       source_location{1, 12}},
            std::tuple{R"_(('&%$#"!~}|{zyxwvuysrqpnKmlkjihgfedcba`_^]\[ZYXWVT1|)_"s,
                       source_location{1, 19}},
            std::tuple{R"_(('&%$#"!~}|{z
yxwvuysrqpnKmlkjihgfedcba`_^]\[ZYXWVT1|)_"s,
                       source_location{2, 6}},
            std::tuple{"\x18"s,
                       source_location{1, 1}},
        }
    );
}

BOOST_AUTO_TEST_CASE(denormalise_valid_test)
{
    auto f = [](auto&& expected, auto&& source) {
        // Iterator version
        {
            auto input = source;
            auto it = denormalise_source(input.begin(), input.end());

            input.erase(it, input.end());
            BOOST_CHECK_EQUAL(input, expected);
        }

        // Range version
        {
            auto input = source;
            auto it = denormalise_source(input);

            input.erase(it, input.end());
            BOOST_CHECK_EQUAL(input, expected);
        }

        // Resize version
        {
            auto input = source;
            denormalise_source_resize(input);
            BOOST_CHECK_EQUAL(input, expected);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{R"_((=BA#9"=<;:3y7x54-21q/p-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       "jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s},
            std::tuple{R"_(('&%#^"!~}{XE)_"s,
                       "jjjj*<jjjj*<v"s},
            std::tuple{R"_(('&%$#"!~}|{zyxwvutsrqpnKmlkjihgfedcba`_^]\[ZYXWVT1|)_"s,
                       "jjjjjjjjjjjjjjjjjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s},
            std::tuple{""s,
                       ""s},
            std::tuple{R"_(('&%$#"!~}|{zyxwvutsrqpnKmlkjihgfedcba`_^]\[ZYXWVT1|)_"s,
                       "jjjjjjjjjjjjjjjjjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v \n\t"s},
        }
    );
}

BOOST_AUTO_TEST_CASE(denormalise_invalid_test)
{
    auto f = [](auto&& source, auto&& source_loc) {
        // Iterator version
        try {
            auto input = source;
            denormalise_source(input.begin(), input.end());
            BOOST_FAIL("Should have thrown");
        } catch (parse_exception& e) {
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), source_loc);
        }

        // Range version
        try {
            auto input = source;
            denormalise_source(input);
            BOOST_FAIL("Should have thrown");
        } catch (parse_exception& e) {
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), source_loc);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"jpoo*pjoooop*ojoopoo*ojoeooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       source_location{1, 25}},
            std::tuple{"jjjj*<jj jj*<v"s,
                       source_location{1, 9}},
            std::tuple{"jjjjjjjjjjjj2jjjjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s,
                       source_location{1, 13}},
            std::tuple{"jjjjjjjjjjjjjjj\njjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s,
                       source_location{1, 16}},
        }
    );
}

BOOST_AUTO_TEST_CASE(end_to_end)
{
    const auto hello_world = R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>)"s;
    auto result = hello_world;

    normalise_source(result);
    BOOST_TEST_MESSAGE("Normalised: " << result);
    denormalise_source(result);

    BOOST_CHECK_EQUAL(hello_world, result);
}

BOOST_AUTO_TEST_CASE(is_likely_normalised_test)
{
    auto f = [](auto&& source, auto&& expected) {
        // Iterator version
        {
            const auto r = is_likely_normalised_source(source.begin(),
                                                       source.end());
            BOOST_CHECK_EQUAL(r, expected);
        }

        // Range version
        {
            const auto r = is_likely_normalised_source(source);
            BOOST_CHECK_EQUAL(r, expected);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       true},
            std::tuple{R"_(('&%#^"!~}{XE)_"s,
                       false},
            std::tuple{"jjjjjjjjjjjjjjjjdjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s,
                       false},
            std::tuple{""s,
                       true},
            std::tuple{"jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji \t\n"s,
                       true},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
