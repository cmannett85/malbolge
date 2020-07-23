/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <iostream>

namespace malbolge
{
/** Namespace for logging types and functions. 
 */
namespace log
{
namespace detail
{
std::ostream& log_prefix(std::ostream& stream);
}

/** Log level.
 */
enum level {
    VERBOSE_DEBUG,  ///< Verbose debug logging level
    DEBUG,          ///< Debug logging level
    INFO,           ///< Informational logging level
    ERROR,          ///< Error logging level
    NUM_LOG_LEVELS  ///< Number of log levels
};

/** String conversion for @a lvl.
 *
 * @param lvl Log level
 * @return String version
 */
const char* to_string(level lvl);

/** Textual streaming operator for level.
 *
 * @param stream Textual output stream
 * @param lvl Log level
 * @return @a stream
 */
inline std::ostream& operator<<(std::ostream& stream, level lvl)
{
    return stream << to_string(lvl);
}

/** Returns the current minimum logging level.
 *
 * @return Minimum logging level
 */
level log_level();

/** Set the minimum logging level.
 *
 * @param lvl New minimum log level.
 */
void set_log_level(level lvl);

/** Prints the log message.
 *
 * Pass your message components as if there is streaming operator where each
 * comma is e.g.:
 * @code
 * log::print(log::INFO, "Failed with number: ", 42, "!");
 * // Prints "Failed with number: 42!"
 * @endcode
 *
 * This is a no-op if @a lvl is less than log_level().
 *
 * @tparam Args Message argument types
 * @param lvl Log level
 * @param args Message arguments
 */
template <typename... Args>
void print(level lvl, Args&&... args)
{
    static_assert(sizeof...(Args) > 0, "Must be at least one argument");

    if (lvl >= log_level()) {
        ((std::clog << detail::log_prefix) << ... << std::forward<Args>(args))
                   << std::endl;
    }
}
}
}
