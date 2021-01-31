/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/debugger/script_runner.hpp"

#include <istream>
#include <filesystem>

namespace malbolge
{
namespace debugger
{
namespace script
{
/** Parse a debugger script provided by @a stream.
 *
 * @note This does not validate the sequence as that is done when the sequence
 * is ran
 * @param stream Input stream providing the debugger script
 * @return Function sequence
 * @exception parse_exception Thrown if the parsing failed
 */
[[nodiscard]]
functions::sequence parse(std::istream& stream);

/** Parse a debugger script loaded from @a path.
 *
 * @note This does not validate the sequence as that is done when the sequence
 * is ran
 * @param path Path to the debugger script
 * @return Function sequence
 * @exception parse_exception Thrown if the parsing failed
 * @exception system_exception Thrown if @a path could not be opened
 */
[[nodiscard]]
functions::sequence parse(const std::filesystem::path& path);
}
}
}
