/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <istream>
#include <optional>

namespace malbolge
{
namespace utility
{
/** A blocking function that extracts the next character from an input stream.
 *
 * This function exists to hide the complexity of extracting a character from
 * browser Javascript in Emscripten, this function needs to block the WASM code
 * but not block the JS runtime in the browser (this is achieved by Asyncify).
 *
 * When not building for WASM this function is equivalent to:
 * @code
 * auto c = char{};
 * return str.get(c) ? std::optional<char>{c} : std::optional<char>{};
 * @endcode
 *
 * @param str Input stream
 * @return Extracted character or empty optional if error
 */
std::optional<char> get_char(std::istream& str);
}
}
