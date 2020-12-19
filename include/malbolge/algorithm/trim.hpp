/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <string_view>

namespace malbolge
{
namespace algorithm
{
/** Trim left function that only moves the begin iterator forwards rather than
 * removing elements.
 *
 * The Predicate signature should be equivalent to:
 * @code
 * bool p(const T&); // Where T is the dereferenced type of Iter
 * @endcode
 * @tparam Iter Iterator type
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @param begin Iterator to begin searching from, and update
 * @param end One-past-the-end of the range iterator
 * @param p Predicate instance
 */
template <typename Iter, typename Predicate>
void trim_left(Iter& begin, Iter end, Predicate&& p)
{
    for ( ; begin != end; ++begin) {
        if (!p(*begin)) {
            return;
        }
    }
}

/** Trim right function that only moves the end iterator rather than removing
 * elements.
 *
 * The Predicate signature should be equivalent to:
 * @code
 * bool p(const T&); // Where T is the dereferenced type of Iter
 * @endcode
 * @tparam Iter Iterator type
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @param begin Iterator to begin searching from
 * @param end One-past-the-end of the range iterator, which is updated
 * @param p Predicate instance
 */
template <typename Iter, typename Predicate>
void trim_right(Iter begin, Iter& end, Predicate&& p)
{
    for ( ; begin != end; --end) {
        if (!p(*(end-1))) {
            return;
        }
    }
}

/** Trim function that only moves the iterators rather than removing elements.
 *
 * The Predicate signature should be equivalent to:
 * @code
 * bool p(const T&); // Where T is the dereferenced type of Iter
 * @endcode
 * @tparam Iter Iterator type
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @param begin Iterator to begin searching from, and update
 * @param end One-past-the-end of the range iterator, which is updated
 * @param p Predicate instance
 */
template <typename Iter, typename Predicate>
void trim(Iter& begin, Iter& end, Predicate p)
{
    trim_left(begin, end, p);
    trim_right(begin, end, p);
}

/** string_view overload of trim_left(Iter&, Iter, Predicate).
 *
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @tparam CharT Char type of @a str
 * @tparam Traits Character traits of @a str
 * @param str string_view to trim
 * @param p Predicate instances
 */
template <typename Predicate, typename CharT, typename Traits>
void trim_left(std::basic_string_view<CharT, Traits>& str, Predicate&& p)
{
    auto begin = str.begin();
    auto end = str.end();
    trim_left(begin, end, std::forward<Predicate>(p));

    str = {&(*begin), static_cast<std::size_t>(std::distance(begin, end))};
}

/** string_view overload of trim_right(Iter, Iter&, Predicate).
 *
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @tparam CharT Char type of @a str
 * @tparam Traits Character traits of @a str
 * @param str string_view to trim
 * @param p Predicate instances
 */
template <typename Predicate, typename CharT, typename Traits>
void trim_right(std::basic_string_view<CharT, Traits>& str, Predicate&& p)
{
    auto begin = str.begin();
    auto end = str.end();
    trim_right(begin, end, std::forward<Predicate>(p));

    str = {&(*begin), static_cast<std::size_t>(std::distance(begin, end))};
}

/** string_view overload of trim(Iter&, Iter&, Predicate).
 *
 * @tparam Predicate Callable that returns true if the element is to be trimmed
 * @tparam CharT Char type of @a str
 * @tparam Traits Character traits of @a str
 * @param str string_view to trim
 * @param p Predicate instances
 */
template <typename Predicate, typename CharT, typename Traits>
void trim(std::basic_string_view<CharT, Traits>& str, Predicate&& p)
{
    auto begin = str.begin();
    auto end = str.end();
    trim(begin, end, std::forward<Predicate>(p));

    str = {&(*begin), static_cast<std::size_t>(std::distance(begin, end))};
}
}
}
