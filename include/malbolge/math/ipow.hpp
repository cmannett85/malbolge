/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <boost/mp11.hpp>

namespace malbolge
{
namespace math
{
/** Compile-time integral exponentiation.
 *
 * Raises @a Base to the power @a Expo.
 * @tparam R Return type
 * @tparam Base Base value
 * @tparam Expo Exponent value
 * @return @a Base raised to the power @a Expo
 */
template <typename R, auto Base, auto Expo>
[[nodiscard]]
constexpr R ipow() noexcept
{
    static_assert(std::is_unsigned_v<R> &&
                  std::is_unsigned_v<decltype(Base)> &&
                  std::is_unsigned_v<decltype(Expo)>,
                  "All parameters must be unsigned integrals");

    using seq = boost::mp11::mp_drop_c<boost::mp11::mp_iota_c<Expo+1>, 1>;

    auto result = R{1};
    boost::mp11::mp_for_each<seq>([&](auto) {
        result *= Base;
    });

    return result;
}

/** Overload that allows for runtime integral exponentiation.
 *
 * Raises @a base to the power @a expo.
 * @tparam R Return type
 * @tparam B Base type
 * @tparam E Exponent type
 * @param base Base value
 * @param expo Exponent value
 * @return @a base raised to the power @a expo
 */
template <typename R, typename B, typename E>
[[nodiscard]]
constexpr R ipow(B&& base, E&& expo) noexcept
{
    static_assert(std::is_unsigned_v<std::decay_t<R>> &&
                  std::is_unsigned_v<std::decay_t<B>> &&
                  std::is_unsigned_v<std::decay_t<E>>,
                  "All parameters must be unsigned integrals");

    auto result = R{1};
    for (auto i = 1u; i <= expo; ++i) {
        result *= base;
    }

    return result;
}
}
}
