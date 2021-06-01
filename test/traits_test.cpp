/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/traits.hpp"

#include "test_helpers.hpp"

#include <variant>
#include <vector>

using namespace malbolge;

namespace
{
template <typename T>
using reserve_checker = decltype(std::declval<T&>().reserve(std::declval<std::size_t>()));

template <typename T>
using value_member_checker = decltype(std::declval<T&>().value);

namespace to_undelying_type_test_enums
{
enum e_type
{
    A,
    B,
    C
};

enum e_set_type : std::uint8_t
{
    D,
    E,
    F
};


enum class e_class_type
{
    G,
    H,
    I
};

enum class e_class_set_type : std::uint16_t
{
    J,
    K,
    L
};
}
}

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

BOOST_AUTO_TEST_CASE(to_underlying_type_test)
{
    using namespace to_undelying_type_test_enums;

    static_assert(traits::to_underlying_type(A) == 0,
                  "Should be the same");
    static_assert(std::is_same_v<decltype(traits::to_underlying_type(A)),
                                 std::underlying_type_t<e_type>>,
                  "Should be the same");
    static_assert(traits::to_underlying_type(C) == 2,
                  "Should be the same");
    static_assert(std::is_same_v<decltype(traits::to_underlying_type(C)),
                                 std::underlying_type_t<e_type>>,
                  "Should be the same");
    static_assert(traits::to_underlying_type(E) == 1,
                  "Should be the same");
    static_assert(std::is_same_v<decltype(traits::to_underlying_type(E)),
                                 std::underlying_type_t<e_set_type>>,
                  "Should be the same");
    static_assert(traits::to_underlying_type(e_class_type::I) == 2,
                  "Should be the same");
    static_assert(std::is_same_v<decltype(traits::to_underlying_type(e_class_type::I)),
                                 std::underlying_type_t<e_class_type>>,
                  "Should be the same");
    static_assert(traits::to_underlying_type(e_class_set_type::K) == 1,
                  "Should be the same");
    static_assert(std::is_same_v<decltype(traits::to_underlying_type(e_class_set_type::K)),
                                 std::underlying_type_t<e_class_set_type>>,
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

BOOST_AUTO_TEST_CASE(is_detected_test)
{
    static_assert(traits::is_detected<reserve_checker, std::vector<char>>::value,
                  "Should be detected");
    static_assert(traits::is_detected<reserve_checker, std::string>::value,
                  "Should be detected");
    static_assert(!traits::is_detected<reserve_checker, std::true_type>::value,
                  "Should not be detected");
    static_assert(!traits::is_detected<reserve_checker, double>::value,
                  "Should not be detected");
    static_assert(!traits::is_detected<value_member_checker, std::vector<char>>::value,
                  "Should be detected");
    static_assert(!traits::is_detected<value_member_checker, std::string>::value,
                  "Should be detected");
    static_assert(traits::is_detected<value_member_checker, std::true_type>::value,
                  "Should not be detected");
    static_assert(!traits::is_detected<value_member_checker, double>::value,
                  "Should not be detected");

    static_assert(traits::is_detected_v<reserve_checker, std::vector<char>>,
                  "Should be detected");
    static_assert(traits::is_detected_v<reserve_checker, std::string>,
                  "Should be detected");
    static_assert(!traits::is_detected_v<reserve_checker, std::true_type>,
                  "Should not be detected");
    static_assert(!traits::is_detected_v<reserve_checker, double>,
                  "Should not be detected");
    static_assert(!traits::is_detected_v<value_member_checker, std::vector<char>>,
                  "Should be detected");
    static_assert(!traits::is_detected_v<value_member_checker, std::string>,
                  "Should be detected");
    static_assert(traits::is_detected_v<value_member_checker, std::true_type>,
                  "Should not be detected");
    static_assert(!traits::is_detected_v<value_member_checker, double>,
                  "Should not be detected");
}

BOOST_AUTO_TEST_SUITE_END()
