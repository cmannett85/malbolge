/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/math/ipow.hpp"
#include "malbolge/traits.hpp"

#include "test_helpers.hpp"

#include <boost/mp11.hpp>

#include <utility>

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(ipow_suite)

BOOST_AUTO_TEST_CASE(squares)
{
    using data_set = std::tuple<
        std::pair<traits::integral_constant<0u>,  traits::integral_constant<1u>>,
        std::pair<traits::integral_constant<1u>,  traits::integral_constant<2u>>,
        std::pair<traits::integral_constant<2u>,  traits::integral_constant<4u>>,
        std::pair<traits::integral_constant<3u>,  traits::integral_constant<8u>>,
        std::pair<traits::integral_constant<4u>,  traits::integral_constant<16u>>,
        std::pair<traits::integral_constant<10u>, traits::integral_constant<1024u>>
    >;

    auto f = [](auto expo, auto result) {
        constexpr auto r = math::ipow<std::uint32_t, 2u, decltype(expo)::value>();
        BOOST_CHECK_EQUAL(r, decltype(result)::value);
    };

    test::data_set(f, data_set{});
}

BOOST_AUTO_TEST_CASE(cubes)
{
    using data_set = std::tuple<
        std::pair<traits::integral_constant<0u>,  traits::integral_constant<1u>>,
        std::pair<traits::integral_constant<1u>,  traits::integral_constant<3u>>,
        std::pair<traits::integral_constant<2u>,  traits::integral_constant<9u>>,
        std::pair<traits::integral_constant<3u>,  traits::integral_constant<27u>>,
        std::pair<traits::integral_constant<4u>,  traits::integral_constant<81u>>,
        std::pair<traits::integral_constant<10u>, traits::integral_constant<59049u>>
    >;

    auto f = [](auto expo, auto result) {
        constexpr auto r = math::ipow<std::uint32_t, 3u, decltype(expo)::value>();
        BOOST_CHECK_EQUAL(r, decltype(result)::value);
    };

    test::data_set(f, data_set{});
}

BOOST_AUTO_TEST_CASE(cubes_runtime)
{
    auto f = [](auto expo, auto result) {
        const auto r = math::ipow<std::uint32_t>(3u, expo);
        BOOST_CHECK_EQUAL(r, result);
    };

    test::data_set(
        f,
        {
            std::tuple{0u, 1u},
            std::tuple{1u, 3u},
            std::tuple{2u, 9u},
            std::tuple{3u, 27u},
            std::tuple{4u, 81u},
            std::tuple{10u, 59049u},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
