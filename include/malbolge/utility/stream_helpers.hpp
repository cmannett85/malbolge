/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include <istream>

namespace malbolge
{
namespace utility
{
/** Returns true if there is data available to read on @a stream.
 *
 * As expected, this will not change the current read position of stream.
 *
 * @note This will return false if <TT>!stream</TT> is true.
 * @tparam CharT Character type of stream
 * @tparam Traits Character trait type
 * @param stream Input stream to test
 * @return True if there is buffered data
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
bool data_available(std::basic_istream<CharT, Traits>& stream)
{
    const auto orig_pos = stream.tellg();
    stream.seekg(0, stream.end);

    const auto buf_size = stream.tellg() - orig_pos;
    stream.seekg(orig_pos, stream.beg);

    return buf_size > 0;
}
}
}
