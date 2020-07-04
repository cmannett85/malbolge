/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <algorithm>

namespace malbolge
{
/** Removes an element from a range by moving the all the higher elements down
 * one.
 *
 * This will leave the last element in an undefined state.
 *
 * @tparam ForwardIt Forward input iterator type
 * @param it Element to remove
 * @param last One-past-the-end iterator
 * @return The updated @a last
 */
template <typename ForwardIt>
ForwardIt remove_from_range(ForwardIt it, ForwardIt last)
{
    auto first = it;
    return std::move(++first, last, it);
}
}
