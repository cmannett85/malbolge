/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/traits.hpp"

#include "test_helpers.hpp"

#include <variant>

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(traits_suite)

BOOST_AUTO_TEST_CASE(always_false_test)
{
    static_assert(!traits::always_false<std::integral_constant<bool, true>>::value,
                  "Should resolve to false");
    static_assert(!traits::always_false<std::integral_constant<bool, false>>::value,
                  "Should resolve to false");
    static_assert(!traits::always_false<double>::value,
                  "Should resolve to false");
    static_assert(!traits::always_false<std::string>::value,
                  "Should resolve to false");

    static_assert(!traits::always_false_v<std::integral_constant<bool, true>>,
                  "Should resolve to false");
    static_assert(!traits::always_false_v<std::integral_constant<bool, false>>,
                  "Should resolve to false");
    static_assert(!traits::always_false_v<double>,
                  "Should resolve to false");
    static_assert(!traits::always_false_v<std::string>,
                  "Should resolve to false");
}

BOOST_AUTO_TEST_CASE(integral_constant_test)
{
    static_assert(std::is_same_v<traits::integral_constant<42>,
                                 std::integral_constant<int, 42>>,
                  "Should be the same");
    static_assert(std::is_same_v<traits::integral_constant<42u>,
                                 std::integral_constant<unsigned int, 42>>,
                  "Should be the same");
    static_assert(std::is_same_v<traits::integral_constant<static_cast<std::uint8_t>(42)>,
                                 std::integral_constant<std::uint8_t, 42>>,
                  "Should be the same");
}

BOOST_AUTO_TEST_CASE(is_tuple_like_test)
{
    static_assert(traits::is_tuple_like<std::pair<int, double>>::value,
                  "Should be tuple-like");
    static_assert(traits::is_tuple_like<std::array<int, 4>>::value,
                  "Should be tuple-like");
    static_assert(traits::is_tuple_like<std::tuple<int, double>>::value,
                  "Should be tuple-like");
    static_assert(!traits::is_tuple_like<std::string>::value,
                  "Should not be tuple-like");

    static_assert(traits::is_tuple_like_v<std::pair<int, double>>,
                  "Should be tuple-like");
    static_assert(traits::is_tuple_like_v<std::array<int, 4>>,
                  "Should be tuple-like");
    static_assert(traits::is_tuple_like_v<std::tuple<int, double>>,
                  "Should be tuple-like");
    static_assert(!traits::is_tuple_like_v<std::string>,
                  "Should not be tuple-like");
}

BOOST_AUTO_TEST_CASE(is_specialisation_test)
{
    static_assert(traits::is_specialisation<std::pair<int, double>, std::pair>::value,
                  "Should be a specialisation");
    static_assert(traits::is_specialisation<std::tuple<int, double>, std::tuple>::value,
                  "Should be a specialisation");
    static_assert(!traits::is_specialisation<std::vector<int>, std::tuple>::value,
                  "Should not be a specialisation");

    static_assert(traits::is_specialisation_v<std::pair<int, double>, std::pair>,
                  "Should be a specialisation");
    static_assert(traits::is_specialisation_v<std::tuple<int, double>, std::tuple>,
                  "Should be a specialisation");
    static_assert(!traits::is_specialisation_v<std::vector<int>, std::tuple>,
                  "Should not be a specialisation");
}

BOOST_AUTO_TEST_CASE(arg_extractor_test)
{
    static_assert(std::is_same_v<traits::arg_extractor<std::tuple<int, double>>,
                                 std::tuple<int, double>>,
                  "Failed to extract correct arguments");
    static_assert(std::is_same_v<traits::arg_extractor<std::vector<int>>,
                                 std::tuple<int, std::allocator<int>>>,
                  "Failed to extract correct arguments");
    static_assert(std::is_same_v<traits::arg_extractor<std::variant<int, double>>,
                                 std::tuple<int, double>>,
                  "Failed to extract correct arguments");
    static_assert(std::is_same_v<traits::arg_extractor<double>,
                                 std::tuple<>>,
                  "Failed to extract correct arguments");
}

BOOST_AUTO_TEST_SUITE_END()
