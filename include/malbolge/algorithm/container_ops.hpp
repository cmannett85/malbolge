/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <algorithm>

namespace malbolge
{
/** Namespace for generic algorithms (in the STL meaning).
 */
namespace algorithm
{
/** Returns true if @a c contains @a value.
 *
 * @tparam Container Container type
 * @tparam Value Value type to search for
 * @param c Container to search
 * @param value Value to find in @a c
 * @return True if @a c contains @a value
 */
template <typename Container, typename Value>
bool any_of(const Container& c, Value&& value)
{
    using std::begin;
    using std::end;
    return std::any_of(begin(c), end(c), [&](auto&& v) { return v == value; });
}

/** Returns true if @a c contains any value in @a value_container.
 *
 * This is a naive implementation, if your containers are sorted then use
 * std::set_intersection as it is much faster.
 * @tparam Container Container type
 * @tparam ValueContainer Container type containing the values to search for
 * @param c Container to search
 * @param value_container Values to find in @a c
 * @return True if @a c contains any value in @a value_container
 */
template <typename Container, typename ValueContainer>
bool any_of_container(const Container& c, ValueContainer&& value_container)
{
    using std::begin;
    using std::end;
    return std::any_of(begin(c), end(c), [&](auto&& v) {
        return any_of(value_container, v);
    });
}
}
}
