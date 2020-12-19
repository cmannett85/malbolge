/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/from_chars.hpp"

#include <cstdlib>
#include <cmath>

using namespace malbolge;

template <>
float utility::from_chars<float>(std::string_view str)
{
    if (str.empty()) {
        throw std::invalid_argument{"Empty from_chars input string"};
    }

    char* end;
    const auto output = std::strtof(str.data(), &end);
    if (output == HUGE_VALF) {
        throw std::out_of_range{"Result not representatable by T"};
    } else if (end == str.data()) {
        throw std::invalid_argument{"Unable to convert using from_chars"};
    }

    return output;
}

template <>
double utility::from_chars<double>(std::string_view str)
{
    if (str.empty()) {
        throw std::invalid_argument{"Empty from_chars input string"};
    }

    char* end;
    const auto output = std::strtod(str.data(), &end);
    if (output == HUGE_VAL) {
        throw std::out_of_range{"Result not representatable by T"};
    } else if (end == str.data()) {
        throw std::invalid_argument{"Unable to convert using from_chars"};
    }

    return output;
}

template <>
math::ternary utility::from_chars<math::ternary>(std::string_view str)
{
    // If this is a non-trit conversion, start by converting to decimal
    if (!str.starts_with("t")) {
        return from_chars<math::ternary::underlying_type>(str);
    }

    str.remove_prefix(1);
    return math::ternary::tritset_type{str};
}
