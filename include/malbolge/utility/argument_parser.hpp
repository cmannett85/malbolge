/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/log.hpp"

#include <optional>
#include <filesystem>

namespace malbolge
{
/** Parses the command line arguments and stores them with an easy to use API.
 */
class argument_parser
{
public:
    /** Constructor.
     *
     * Parses the command line arguments.
     * @param argc Argument count
     * @param argv Arguments
     * @throw system_exception Thrown if invalid arguments were provided
     */
    explicit argument_parser(int argc, char* argv[]);

    /** Returns true if help was requested.
     *
     * @return True if help requested
     */
    bool help() const
    {
        return help_;
    }

    /** Returns true if the application version was requested.
     *
     * @return True if version requested
     */
    bool version() const
    {
        return version_;
    }

    /** Returns the input file path.
     *
     * @return Input file path, or an empty optional if not file was provided
     */
    const std::optional<std::filesystem::path>& file() const
    {
        return file_;
    }

    /** Returns the logging level.
     *
     * @return Logging level
     */
    log::level log_level() const
    {
        return log_level_;
    }

private:
    bool help_;
    bool version_;
    std::optional<std::filesystem::path> file_;
    log::level log_level_;
};

/** Prints the help output into @a stream.
 *
 * @param stream Textual output stream
 * @param parser Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, const argument_parser& parser);
}
