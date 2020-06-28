/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <type_traits>

namespace malbolge
{
namespace traits
{
/** Regardless of @a T, always yields a false.
 *
 * Useful for forcing a static_assert failure, as you just putting false in
 * the conditional field does not work.
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
using integral_constant = std::integral_constant<std::decay_t<decltype(Value)>, Value>;
}
}
