/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/traits.hpp"

#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/preprocessor/config/limits.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

#include <array>
#include <string_view>

/** @file
 */

namespace malbolge
{
namespace utility
{
/** Simple compile-time string constant, allows a string to be used as a
 * template parameter.
 *
 * @tparam Cs Char sequence
 */
template <char... Cs>
class string_constant
{
public:
    /** Container's value type.
     *
     * Always char, no wide-string support for now.
     */
    using value_type = char;

    /** Size type.
     */
    using size_type = std::size_t;

    /** Returns the number of characters in the string.
     *
     * @return Number of characters
     */
    static constexpr size_type size()
    {
        return traits::integral_constant<sizeof...(Cs)>::value;
    }

    /** Returns a string_view for runtime use.
     *
     * This points to static data.
     * @return string_view equivalent
     */
    static constexpr std::string_view value()
    {
        return {data_.data(), data_.size()};
    }

    /** Returns the character at @a pos at compiler time.
     *
     * It is a compilation failure if @a Pos is greater than or equal to size().
     * @tparam Pos Position/index to retrieve
     * @return Character at @a Pos
     */
    template <size_type Pos>
    static constexpr char at()
    {
        return std::get<Pos>(data_);
    }

    /** Returns the character at @a pos.
     *
     * If @a pos it out of range, runtime a std::out_of_range is thrown.
     * @param pos Position/index to retrieve
     * @return Character at @a pos
     * @exception std::out_of_range Thrown if a runtime range check fails
     */
    constexpr char at(size_type pos) const
    {
        return data_.at(pos);
    }

    /** Returns the character at @a pos.
     *
     * It is undefined behaviour is @a pos is greater than or equal to size().
     * Use at(size_type) for runtime checking.
     * @param pos Position/index to retrieve
     * @return Character at @a pos
     */
    constexpr char operator[](size_type pos) const
    {
        return data_[pos];
    }

private:
    static constexpr auto data_ = std::array<value_type, size()>{Cs...};
};

namespace detail
{
template <int N>
constexpr char get(const char(&str)[N], int i)
{
    return i < N ? str[i] : '\0';
}

struct is_null_char
{
    template <typename T>
    using fn = std::is_same<traits::integral_constant<'\0'>, T>;
};

template <char... Cs>
class builder
{
    using strip_null = boost::mp11::mp_remove_if_q<
        boost::mp11::mp_list_c<char, Cs...>,
        is_null_char
    >;

    template <char... Stripped>
    static constexpr auto list_to_string(boost::mp11::mp_list_c<char, Stripped...>)
    {
        return string_constant<Stripped...>{};
    }

public:
    using type = decltype(list_to_string(strip_null{}));
};

#define MAL_STR_CHAR(z, n, tok) \
    BOOST_PP_COMMA_IF(n) malbolge::utility::detail::get(tok, n)

#define MAL_STR_N(n, tok) \
    typename malbolge::utility::detail::builder<BOOST_PP_REPEAT(n, MAL_STR_CHAR, tok)>::type
}

/** Macro to create a string_constant from a literal.
 *
 * Due to limitations in C++ we cannot use a user-defined string literal
 * function to create a type based upon a parameter pack of chars (e.g.
 * <TT>"hello"_str</TT>).  So the simplest approach is to use the preprocessor
 * to write out the template char arguments for us.
 *
 * There is no requirement to use this, it just makes schema definitions a
 * little easier to read.
 * @note There is a limit of 128 characters
 */
#define MAL_STR(tok) MAL_STR_N(128, #tok)
}
}
