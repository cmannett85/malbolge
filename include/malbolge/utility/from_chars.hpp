/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/ternary.hpp"

#include <charconv>
#include <string_view>
#include <stdexcept>

namespace malbolge
{
namespace utility
{
/** A simple wrapper over std::from_chars for integral types that allows the
 * base to be set.
 *
 * Any leading '+' character is stripped.
 *
 * @tparam Integral type to convert to
 * @param str String to convert from
 * @param base Numerical base of value represented by @a str
 * @return The conversion result
 * @exception std::invalid_argument Thrown if @a str is empty, or the conversion
 * failed
 * @exception std::out_of_range Thrown if @a str is not representable by @a T
 * @exception std::runtime_error Thrown if an unknown conversion error occurred
 */
template <typename T>
T from_chars_with_base(std::string_view str, std::uint8_t base)
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");

    if (str.empty()) {
        throw std::invalid_argument{"Empty from_chars input string"};
    }

    // Strip a leading plus sign
    if (str.starts_with("+")) {
        str.remove_prefix(1);
    }

    auto output = T{};
    const auto [p, ec] = std::from_chars(str.data(),
                                         str.data() + str.size(),
                                         output,
                                         base);
    if (ec != std::errc{}) {
        if (ec == std::errc::invalid_argument) {
            throw std::invalid_argument{"Unable to convert using from_chars"};
        } else if (ec == std::errc::result_out_of_range) {
            throw std::out_of_range{"Result not representatable by T"};
        }
        // The library documentation suggests it shouldn't get here, but just
        // in case...
        throw std::runtime_error{"Unknown from_chars conversion error"};
    }

    return output;
}

/** A simple wrapper over std::from_chars for common scenarios.
 *
 * The formatting is determined by the prefix:
 * - 0x or 0X is base-16 or std::chars_fmt::hex
 * - 0 is base-8, integers only
 * - t is trit, ternary only
 * Scientific or fixed exponent for floating point is determined by the content
 * of @a str.  Any leading '+' character is stripped.
 *
 * @tparam Numeric type to convert to
 * @param str String to convert from
 * @return The conversion result
 * @exception std::invalid_argument Thrown if @a str is empty, or the conversion
 * failed
 * @exception std::out_of_range Thrown if @a str is not representable by @a T
 * @exception std::runtime_error Thrown if an unknown conversion error occurred
 */
template <typename T>
T from_chars(std::string_view str)
{
    if (str.empty()) {
        throw std::invalid_argument{"Empty from_chars input string"};
    }

    // Strip a leading plus sign
    if (str.starts_with("+")) {
        str.remove_prefix(1);
    }

    // Determine base
    auto base = 10;
    if (str.starts_with("0x") || str.starts_with("0X")) {
        str.remove_prefix(2);
        base = 16;
    } else if (str.starts_with("0")) {
        base = 8;
    }

    return from_chars_with_base<T>(str, base);
}

// No floating point from_chars support in gcc 10.2, have to fall back to
// the older methods
template <>
float from_chars<float>(std::string_view str);

template <>
double from_chars<double>(std::string_view str);

// Ternary specialisation
template <>
math::ternary from_chars<math::ternary>(std::string_view str);
}
}
