/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/math/ternary.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace malbolge::literals;

BOOST_AUTO_TEST_SUITE(ternary_suite)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    BOOST_CHECK_EQUAL(math::ternary{}, 0u);
    BOOST_CHECK_EQUAL(math::ternary::max, 59048u);
}

BOOST_AUTO_TEST_CASE(constructor)
{
    auto f = [](auto v, auto expected) {
        BOOST_CHECK_EQUAL(math::ternary{v}, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{0u, 0u},
            std::tuple{42u, 42u},
            std::tuple{math::ternary::max, math::ternary::max},
            std::tuple{math::ternary::max+1, 0u},
            std::tuple{math::ternary::max+5, 4u},
        }
    );
}

BOOST_AUTO_TEST_CASE(comparisons)
{
    using cmp_sig = std::function<bool (math::ternary, math::ternary)>;

    const auto eq = cmp_sig{[](auto a, auto b) { return a == b; }};
    const auto ne = cmp_sig{[](auto a, auto b) { return a != b; }};
    const auto lt = cmp_sig{[](auto a, auto b) { return a <  b; }};
    const auto le = cmp_sig{[](auto a, auto b) { return a <= b; }};
    const auto gt = cmp_sig{[](auto a, auto b) { return a >  b; }};
    const auto ge = cmp_sig{[](auto a, auto b) { return a >= b; }};

    auto f = [](math::ternary a, math::ternary b, auto&& cmp, auto expected) {
        const auto r = cmp(a, b);
        BOOST_CHECK_EQUAL(r, expected);
    };

    BOOST_TEST_MESSAGE("Equality");
    test::data_set(
        f,
        {
            std::tuple{ 0u,  0u, eq, true},
            std::tuple{ 0u,  0u, ne, false},
            std::tuple{42u, 42u, eq, true},
            std::tuple{42u, 42u, ne, false},
            std::tuple{math::ternary::max-1, math::ternary::max-1, eq, true},
            std::tuple{math::ternary::max-1, math::ternary::max-1, ne, false},
        }
    );

    BOOST_TEST_MESSAGE("Less than");
    test::data_set(
        f,
        {
            std::tuple{ 0u,  0u, lt, false},
            std::tuple{ 0u,  0u, le, true},
            std::tuple{24u, 24u, lt, false},
            std::tuple{24u, 24u, le, true},
            std::tuple{24u, 42u, lt, true},
            std::tuple{24u, 42u, le, true},
            std::tuple{42u, 24u, lt, false},
            std::tuple{42u, 24u, le, false},
        }
    );

    BOOST_TEST_MESSAGE("Greater than");
    test::data_set(
        f,
        {
            std::tuple{ 0u,  0u, gt, false},
            std::tuple{ 0u,  0u, ge, true},
            std::tuple{24u, 24u, gt, false},
            std::tuple{24u, 24u, ge, true},
            std::tuple{24u, 42u, gt, false},
            std::tuple{24u, 42u, ge, false},
            std::tuple{42u, 24u, gt, true},
            std::tuple{42u, 24u, ge, true},
        }
    );
}

BOOST_AUTO_TEST_CASE(to_tritset)
{
    using tritset = math::ternary::tritset_type;

    auto f = [](auto dec) {
        BOOST_CHECK_EQUAL(math::ternary{dec}.to_tritset(), tritset{dec});
    };

    test::data_set(
        f,
        {
            std::tuple{0u},
            std::tuple{42u},
            std::tuple{math::ternary::max},
        }
    );
}

BOOST_AUTO_TEST_CASE(addition)
{
    auto f = [](math::ternary a, math::ternary b, auto expected) {
        BOOST_CHECK_EQUAL(a + b, expected);

        a += b;
        BOOST_CHECK_EQUAL(a, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{ 0u,  0u,  0u},
            std::tuple{42u,  0u, 42u},
            std::tuple{ 0u, 42u, 42u},
            std::tuple{42u, 42u, 84u},
            std::tuple{math::ternary::max, 0u, math::ternary::max},
            std::tuple{math::ternary::max, 1u, 0u},
            std::tuple{math::ternary::max, 5u, 4u},
        }
    );
}

BOOST_AUTO_TEST_CASE(subtraction)
{
    auto f = [](math::ternary a, math::ternary b, auto expected) {
        BOOST_CHECK_EQUAL(a - b, expected);

        a -= b;
        BOOST_CHECK_EQUAL(a, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{ 0u,  0u,  0u},
            std::tuple{42u,  0u, 42u},
            std::tuple{42u, 10u, 32u},
            std::tuple{42u, 42u,  0u},
            std::tuple{math::ternary::max, 0u, math::ternary::max},
            std::tuple{math::ternary::max, 1u, math::ternary::max-1},
            std::tuple{math::ternary::max, 5u, math::ternary::max-5},
        }
    );
}

BOOST_AUTO_TEST_CASE(modulo)
{
    auto f = [](math::ternary a, math::ternary b, auto expected) {
        BOOST_CHECK_EQUAL(a % b, expected);

        a %= b;
        BOOST_CHECK_EQUAL(a, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{ 0u,  5u,  0u},
            std::tuple{42u,  5u,  2u},
            std::tuple{84u, 42u,  0u},
            std::tuple{52u, 42u, 10u},
        }
    );
}

BOOST_AUTO_TEST_CASE(rotate)
{
    auto f = [](math::ternary in, auto r, math::ternary out) {
        in.rotate(r);
        BOOST_CHECK_EQUAL(in, out);
    };

    test::data_set(
        f,
        {
            std::tuple{0000001000_trit,  1, 0000000100_trit},
            std::tuple{0000000222_trit,  2, 2200000002_trit},
            std::tuple{0000000002_trit,  3, 0020000000_trit},
            std::tuple{0000010000_trit,  5, 1000000000_trit},
            std::tuple{0000010002_trit, 14, 0002000001_trit},
            std::tuple{0000000000_trit,  3, 0000000000_trit},
            std::tuple{2222222222_trit,  2, 2222222222_trit},
        }
    );
}

BOOST_AUTO_TEST_CASE(op)
{
    auto f = [](math::ternary a, math::ternary b, math::ternary expected) {
        BOOST_CHECK_EQUAL(a.op(b), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{0000001000_trit, 0000000100_trit, 1111110111_trit},
            std::tuple{0000000222_trit, 2200000002_trit, 2211111001_trit},
            std::tuple{0000000002_trit, 0020000000_trit, 1121111110_trit},
            std::tuple{0000010000_trit, 1000000000_trit, 1111101111_trit},
            std::tuple{0000010002_trit, 0002000001_trit, 1112101112_trit},
            std::tuple{0000000000_trit, 0000000000_trit, 1111111111_trit},
            std::tuple{2222222222_trit, 2222222222_trit, 1111111111_trit},
            std::tuple{0001112220_trit, 0120120120_trit, 1120020211_trit},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
