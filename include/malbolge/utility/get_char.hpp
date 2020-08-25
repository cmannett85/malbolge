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
/** get_char result type.
 */
enum class get_char_result {
    CHAR,       ///< Character extracted from the stream
    NO_DATA,    ///< No data available
    ERROR,      ///< Error on the stream
};

/** A function that extracts the next character from an input stream.
 *
 * This function exists to hide the complexity of extracting a character from
 * browser Javascript in Emscripten, this function needs to block the WASM code
 * but not block the JS runtime in the browser (this is achieved by Asyncify).
 *
 * @param str Input stream
 * @param c Extract character
 * @param block If true, will block on the stream until there is data to read
 * or an error occurs.  If false, will return immediately
 * @return Result type
 */
get_char_result get_char(std::istream& str, char& c, bool block = true);
}
}
