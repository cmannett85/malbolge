/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace malbolge
{
namespace logging
{
/** Log level.
 */
enum level {
    VERBOSE_DEBUG,  /// Verbose debug logging level
    DEBUG,          /// Debug logging level
    INFO,           /// Informational logging level
    ERROR,          /// Error logging level
    NUM_LOG_LEVELS  /// Number of log levels
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

/** Log source type.
 */
using source_type = boost::log::sources::severity_logger_mt<level>;

/** Defines the global logging source instance.
 */
BOOST_LOG_GLOBAL_LOGGER(source, source_type)

/** Initialise the logging system
 *
 * Sets the minimum log level to ERROR.
 */
void init_logging();

/** Set the minimum logging level.
 *
 * @param lvl New minimum log level.
 */
void set_log_level(level lvl);
}
}
