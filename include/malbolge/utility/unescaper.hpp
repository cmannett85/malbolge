/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/utility/from_chars.hpp"

#include <string>
#include <optional>
#include <unordered_map>

namespace malbolge
{
namespace utility
{
namespace detail
{
// These functions are derived from the escape list detailed here:
// https://stackoverflow.com/questions/10220401/rules-for-c-string-literals-escape-character
template <typename Iter>
[[nodiscard]]
std::optional<char> is_control_character(Iter it) noexcept
{
    static const auto mapper = std::unordered_map<char, char>{
        {'a', '\a'},
        {'b', '\b'},
        {'t', '\t'},
        {'n', '\n'},
        {'v', '\v'},
        {'f', '\f'},
        {'r', '\r'},
    };
    auto c_it = mapper.find(*it);
    if (c_it != mapper.end()) {
        return c_it->second;
    }

    return {};
}

template <typename Iter>
[[nodiscard]]
std::optional<char> is_punctuation_character(Iter it) noexcept
{
    static const auto mapper = std::array<char, 4>{'\"', '\'', '\?', '\\'};
    if (std::find(mapper.begin(), mapper.end(), *it) != mapper.end()) {
        return *it;
    }

    return {};
}

// The octal and hex functions are 'greedy' i.e. they move the iterator along
template <typename Iter>
[[nodiscard]]
std::optional<char> is_octal_character(Iter& it, Iter end)
{
    static constexpr auto base_8 = 8u;
    static constexpr auto max_octal_digits = 3u;

    auto count = 0u;
    for (auto c_it = it; (c_it != end) && (count < max_octal_digits); ++c_it) {
        if ((*c_it >= '0') && (*c_it <= '7')) {
            ++count;
        } else {
            break;
        }
    }
    if (count > 0u) {
        const auto str = std::string_view{&(*it), count};
        const auto result = utility::from_chars_with_base<char>(str, base_8);
        it += count - 1; // Move the iterator along to last octal character
        return result;
    }

    return {};
}

template <typename Iter>
[[nodiscard]]
std::optional<char> is_hex_character(Iter& it, Iter end)
{
    static constexpr auto base_16 = 16u;

    if (*it != 'x') {
        return {};
    }

    // Skip the x
    ++it;

    auto count = 0u;
    for (auto c_it = it; c_it != end; ++c_it) {
        if (std::isxdigit(*c_it)) {
            ++count;
        } else {
            break;
        }
    }
    if (count > 0u) {
        const auto str = std::string_view{&(*it), count};
        const auto result = utility::from_chars_with_base<char>(str, base_16);
        it += count - 1; // Move the iterator along to last hex character
        return result;
    }

    return {};
}
}

/** Returns an unescaped copy of @a str.
 *
 * Takes an input that has escaped characters in it, and returns a copy where
 * the escaped characters have been replaced with their 'real' equivalents.
 * E.g.:
 * @code
 * \"hello\" there\? \042 \x42
 * "hello" there? " B
 * @endcode
 * @tparam Traits Character traits
 * @tparam Allocator Allocator type
 * @param str Input string
 * @return Unescaped equivalent
 * @exception std::out_of_range Thrown if a hex value in @a str is not
 * representable by char
 * @exception std::invalid_argument Thrown if an escaped character is malformed
 */
template <typename Traits = std::char_traits<char>,
          typename Allocator = std::allocator<char>>
[[nodiscard]]
std::basic_string<char, Traits, Allocator>
unescape_ascii(std::basic_string_view<char, Traits>& str)
{
    auto unescaped = std::basic_string<char, Traits, Allocator>{};
    unescaped.reserve(str.size());

    for (auto it = str.begin(); it != str.end(); ++it) {
        if (*it == '\\') {
            ++it;

            auto result = detail::is_control_character(it);
            if (result) {
                unescaped += *result;
                continue;
            }

            result = detail::is_punctuation_character(it);
            if (result) {
                unescaped += *result;
                continue;
            }

            result = detail::is_octal_character(it, str.end());
            if (result) {
                unescaped += *result;
                continue;
            }

            result = detail::is_hex_character(it, str.end());
            if (result) {
                unescaped += *result;
                continue;
            }

            throw std::invalid_argument{"Cannot parse escape character"};
        } else {
            unescaped.push_back(*it);
        }
    }

    return unescaped;
}
}
}
