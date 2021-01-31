/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <iostream>
#include <mutex>

namespace malbolge
{
/** Namespace for logging types and functions. 
 */
namespace log
{
namespace detail
{
std::ostream& timestamp(std::ostream& stream);
std::mutex& log_lock();
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
[[nodiscard]]
const char* to_string(level lvl) noexcept;

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
[[nodiscard]]
level log_level() noexcept;

/** Set the minimum logging level.
 *
 * @param lvl New minimum log level
 */
void set_log_level(level lvl) noexcept;

/** ANSI terminal colour constants.
 */
enum class colour
{
    DEFAULT,    ///< Default text colour
    RED,        ///< Red
    GREEN,      ///< Green
    YELLOW,     ///< Yellow
    BLUE,       ///< Blue
    NUM_COLOURS ///< Number of colours
};

namespace detail
{
[[nodiscard]]
std::string_view colour_to_ansi(colour c = colour::DEFAULT) noexcept;

[[nodiscard]]
colour log_level_to_colour(level lvl) noexcept;
}

/** Basic log print functionality that rights the standard prefix into
 * @a stream.
 *
 * Pass your message components as if there is streaming operator where each
 * comma is e.g.:
 * @code
 * log::basic_print(std::cout, " - Hello ", 42, "!");
 * // Prints "2020-12-19 09:57:15.486571467 - Hello 42!"
 * @endcode
 *
 * This function is threadsafe.
 * @tparam Args Message argument types
 * @param stream Output stream
 * @param c Text colour
 * @param args Message arguments, maybe empty in which case just the timestamp
 * is printed
 */
template <typename... Args>
void basic_print(std::ostream& stream, colour c, Args&&... args)
{
    auto lock = std::lock_guard{detail::log_lock()};
    ((stream << detail::colour_to_ansi(c) << detail::timestamp)
        << ... << std::forward<Args>(args))
    << detail::colour_to_ansi() << std::endl;
}

/** Prints the log message.
 *
 * Pass your message components as if there is streaming operator where each
 * comma is e.g.:
 * @code
 * log::print(log::INFO, "Failed with number: ", 42, "!");
 * // Prints "2020-12-19 09:57:15.486571467[INFO]: Failed with number: 42!"
 * @endcode
 * This is a no-op if @a lvl is less than log_level().
 * @tparam Args Message argument types
 * @param lvl Log level
 * @param args Message arguments
 */
template <typename... Args>
void print(level lvl, Args&&... args)
{
    static_assert(sizeof...(Args) > 0, "Must be at least one argument");

    if (lvl >= log_level()) {
        basic_print(std::clog, detail::log_level_to_colour(lvl),
                    "[", lvl, "]: ",
                    std::forward<Args>(args)...);
    }
}
}
}
