/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/math/tritset.hpp"
#include "malbolge/traits.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace malbolge::literals;

BOOST_AUTO_TEST_SUITE(tritset_suite)

BOOST_AUTO_TEST_CASE(constants)
{
    using data_set = std::tuple<
        std::tuple<traits::integral_constant<1u>, traits::integral_constant<2u>>,
        std::tuple<traits::integral_constant<2u>, traits::integral_constant<8u>>,
        std::tuple<traits::integral_constant<3u>, traits::integral_constant<26u>>
    >;

    auto f = [](auto expo, auto max) {
        using type = math::tritset<decltype(expo)::value, std::uint32_t>;

        BOOST_CHECK_EQUAL(type::width, decltype(expo)::value);
        BOOST_CHECK_EQUAL(math::trit::base, 3u);
        BOOST_CHECK_EQUAL(type::max, decltype(max)::value);
        BOOST_CHECK_EQUAL(math::trit::bits_per_trit, 2u);
    };

    test::data_set(f, data_set{});
}

BOOST_AUTO_TEST_CASE(constructor)
{
    using tritset = math::tritset<5u>;

    auto f = [](auto dec, auto trit) {
        auto a = tritset{};
        for (auto i = 0u; i < tritset::width; ++i) {
            a.set(i, trit[i]);
            BOOST_CHECK_EQUAL(a[i], trit[i]);
        }
        BOOST_CHECK_EQUAL(a, trit);
        BOOST_CHECK_EQUAL(a.to_base10(), dec % (tritset::max+1));

        auto b = tritset{static_cast<tritset::underlying_type>(dec)};
        BOOST_CHECK_EQUAL(a, b);
        BOOST_CHECK_EQUAL(b.to_base10(), dec % (tritset::max+1));

        for (auto i = 0u; i < tritset::width; ++i) {
            BOOST_CHECK_EQUAL(a[i], b[i]);
            BOOST_CHECK_EQUAL(b[i], trit[i]);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{27u,  01000_trit},
            std::tuple{26u,  00222_trit},
            std::tuple{2u,   00002_trit},
            std::tuple{81u,  10000_trit},
            std::tuple{83u,  10002_trit},
            std::tuple{243u, 00000_trit},
            std::tuple{242u, 22222_trit},
        }
    );
}

BOOST_AUTO_TEST_CASE(comparisons)
{
    using tritset = math::tritset<5u, std::uint32_t>;
    using cmp_sig = std::function<bool (tritset, tritset)>;

    const auto eq = cmp_sig{[](tritset a, tritset b) { return a == b; }};
    const auto ne = cmp_sig{[](tritset a, tritset b) { return a != b; }};
    const auto lt = cmp_sig{[](tritset a, tritset b) { return a <  b; }};
    const auto le = cmp_sig{[](tritset a, tritset b) { return a <= b; }};
    const auto gt = cmp_sig{[](tritset a, tritset b) { return a >  b; }};
    const auto ge = cmp_sig{[](tritset a, tritset b) { return a >= b; }};

    auto f = [](auto a, auto b, auto&& cmp, auto expected) {
        const auto r = cmp(tritset{a}, tritset{b});
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
            std::tuple{tritset::max-1, tritset::max-1, eq, true},
            std::tuple{tritset::max-1, tritset::max-1, ne, false},
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

BOOST_AUTO_TEST_CASE(rotate)
{
    auto f = [](auto in, auto r, auto out) {
        in.rotate(r);
        BOOST_CHECK_EQUAL(in, out);
    };

    test::data_set(
        f,
        {
            std::tuple{01000_trit, 1, 00100_trit},
            std::tuple{00222_trit, 2, 22002_trit},
            std::tuple{00002_trit, 3, 00200_trit},
            std::tuple{10000_trit, 5, 10000_trit},
            std::tuple{10002_trit, 9, 00021_trit},
            std::tuple{00000_trit, 3, 00000_trit},
            std::tuple{22222_trit, 2, 22222_trit},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
