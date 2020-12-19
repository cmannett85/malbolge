/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <type_traits>
#include <tuple>

namespace malbolge
{
namespace traits
{
/** Regardless of @a T, always yields a false.
 *
 * Useful for forcing a static_assert failure, as just putting false in the
 * conditional field does not work.
 * @tparam T Any dependent argument
 */
template <typename T>
struct always_false : std::false_type
{};

/** Helper variable for always_false.
 *
 * @tparam T Any dependent argument
 */
template <typename T>
constexpr bool always_false_v = always_false<T>::value;

/** Wrapper for std::integral_constant that uses the type of @a Value as the
 * type parameter.
 *
 * @code
 * std::is_same_v<
 *     integral_constant<42u>,
 *     std::integral_constant<unsigned int, 42>
 * > == true
 * @endcode
 * @tparam Value Integral to wrap
 */
template <auto Value>
using integral_constant = std::integral_constant<std::decay_t<decltype(Value)>,
                                                 Value>;

/** Evaluates to true if @a T is a tuple-like type.
 *
 * A tuple-like type is one that is can be used with std::tuple_size (i.e.
 * std::pair, std::array, and std::tuple).
 * @tparam T Type to test
 */
template <typename T, typename = void>
struct is_tuple_like :
    std::false_type
{};

template <typename T>
struct is_tuple_like<T, std::void_t<typename std::tuple_size<T>::type>> :
    std::true_type
{};

/** Helper variable for is_tuple_like.
 *
 * @tparam T Type to test
 */
template <typename T>
constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

/** True if @a T is a specialisation of @a U.
 *
 * @code
 * is_specialisation<std::vector<int>, std::vector> // True
 * is_specialisation<std::vector<int>, std::deque>  // False
 * @endcode
 * @tparam T Type to test
 * @tparam U Unspecialised type to test against
 */
template <typename T, template<typename...> class U>
struct is_specialisation : std::false_type
{};

template <template<typename...> class U, typename... Args>
struct is_specialisation<U<Args...>, U> : std::true_type
{};

/** Helper variable for is_specialisations.
 *
 * @tparam T Type to test
 * @tparam U Unspecialised type to test against
 */
template <typename T, template<typename...> class U>
constexpr bool is_specialisation_v = is_specialisation<T, U>::value;

namespace detail
{
template <template<typename...> class T, typename... Args>
constexpr auto arg_extractor_impl(const T<Args...>&) -> std::tuple<Args...>
{};

template <typename T>
constexpr auto arg_extractor_impl(const T&) -> std::tuple<>
{
    return {};
};
}

/** Alias which is a tuple of the template parameters of @a T.
 *
 * @code
 * std::is_same_v<arg_extractor<std::vector<int>>, std::tuple<int, std::allocator<int>>>;
 * std::is_same_v<arg_extractor<std::variant<int, double>>, std::tuple<int, double>>;
 * std::is_same_v<arg_extractor<double>, std::tuple<>>;
 * @endcode
 * @tparam T Type to test
 * @tparam U Unspecialised type, deducted
 */
template <typename T>
using arg_extractor = std::decay_t<decltype(detail::arg_extractor_impl(T{}))>;
}
}
