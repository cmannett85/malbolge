/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(source_location_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    // Default constructor
    BOOST_CHECK_EQUAL(source_location{}.line, 1);
    BOOST_CHECK_EQUAL(source_location{}.column, 1);

    auto f = [](auto l, auto c) {
        const auto result = source_location{l, c};
        BOOST_CHECK_EQUAL(result.line, l);
        BOOST_CHECK_EQUAL(result.column, c);
    };

    test::data_set(
        f,
        {
            std::tuple{42u, 6u},
            std::tuple{6u, 42u},
        }
    );
}

BOOST_AUTO_TEST_CASE(comparison_operators)
{
    using cmp_sig = std::function<bool (source_location, source_location)>;

    const auto eq = cmp_sig{[](source_location a, source_location b) { return a == b; }};
    const auto ne = cmp_sig{[](source_location a, source_location b) { return a != b; }};
    const auto lt = cmp_sig{[](source_location a, source_location b) { return a <  b; }};
    const auto le = cmp_sig{[](source_location a, source_location b) { return a <= b; }};
    const auto gt = cmp_sig{[](source_location a, source_location b) { return a >  b; }};
    const auto ge = cmp_sig{[](source_location a, source_location b) { return a >= b; }};

    auto f = [](auto a, auto b, auto&& cmp, auto expected) {
        const auto r = cmp(a, b);
        BOOST_CHECK_EQUAL(r, expected);
    };

    BOOST_TEST_MESSAGE("Equality");
    test::data_set(
        f,
        {
            std::tuple{source_location{},  source_location{}, eq, true},
            std::tuple{source_location{},  source_location{}, ne, false},
            std::tuple{source_location{4, 6},  source_location{4, 6}, eq, true},
            std::tuple{source_location{4, 6},  source_location{4, 6}, ne, false},
            std::tuple{source_location{4, 4},  source_location{4, 4}, eq, true},
            std::tuple{source_location{4, 4},  source_location{4, 4}, ne, false},
        }
    );

    BOOST_TEST_MESSAGE("Less than");
    test::data_set(
        f,
        {
            std::tuple{source_location{},  source_location{}, lt, false},
            std::tuple{source_location{},  source_location{}, le, true},
            std::tuple{source_location{4, 4},  source_location{4, 4}, lt, false},
            std::tuple{source_location{4, 4},  source_location{4, 4}, le, true},
            std::tuple{source_location{4, 6},  source_location{4, 4}, lt, false},
            std::tuple{source_location{4, 6},  source_location{4, 4}, le, false},
            std::tuple{source_location{4, 2},  source_location{4, 4}, lt, true},
            std::tuple{source_location{4, 2},  source_location{4, 4}, le, true},
            std::tuple{source_location{3, 4},  source_location{4, 4}, lt, true},
            std::tuple{source_location{3, 4},  source_location{4, 4}, le, true},
            std::tuple{source_location{5, 4},  source_location{4, 4}, lt, false},
            std::tuple{source_location{5, 4},  source_location{4, 4}, le, false},
        }
    );

    BOOST_TEST_MESSAGE("Greater than");
    test::data_set(
        f,
        {
            std::tuple{source_location{},  source_location{}, gt, false},
            std::tuple{source_location{},  source_location{}, ge, true},
            std::tuple{source_location{4, 4},  source_location{4, 4}, gt, false},
            std::tuple{source_location{4, 4},  source_location{4, 4}, ge, true},
            std::tuple{source_location{4, 6},  source_location{4, 4}, gt, true},
            std::tuple{source_location{4, 6},  source_location{4, 4}, ge, true},
            std::tuple{source_location{4, 2},  source_location{4, 4}, gt, false},
            std::tuple{source_location{4, 2},  source_location{4, 4}, ge, false},
            std::tuple{source_location{3, 4},  source_location{4, 4}, gt, false},
            std::tuple{source_location{3, 4},  source_location{4, 4}, ge, false},
            std::tuple{source_location{5, 4},  source_location{4, 4}, gt, true},
            std::tuple{source_location{5, 4},  source_location{4, 4}, ge, true},
        }
    );
}

BOOST_AUTO_TEST_CASE(textual_conversion)
{
    auto f = [](auto src_loc, auto expected) {
        const auto result = to_string(src_loc);
        BOOST_CHECK_EQUAL(result, expected);

        auto ss = std::stringstream{};
        ss << src_loc;
        BOOST_CHECK_EQUAL(ss.str(), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{optional_source_location{}, "{}"},
            std::tuple{optional_source_location{source_location{}}, "{l:1, c:1}"},
            std::tuple{optional_source_location{source_location{4, 6}}, "{l:4, c:6}"},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
