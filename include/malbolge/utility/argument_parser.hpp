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
    /** The program source type enum.
     */
    enum class program_source {
        DISK,   ///< From a file on disk
        STDIN,  ///< From stdin
        STRING, ///< From a string passed as a constructor argument
        MAX     ///< Maximum number of sources
    };

    /** The program data.
     *
     * A pair containing the program source type, and:
     * - If @a source is DISK, @a data is the path
     * - If @a source is STDIN, @a data is empty
     * - If @a source is STRING, @a data is the program source code
     */
    struct program_data {
        program_source source;  ///< Program source
        std::string data;       ///< Data associated with the source type
    };

    /** Constructor.
     *
     * Parses the command line arguments.
     * @param argc Argument count
     * @param argv Arguments
     * @exception system_exception Thrown if invalid arguments were provided
     */
    explicit argument_parser(int argc, char* argv[]);

    /** Returns true if help was requested.
     *
     * @return True if help requested
     */
    [[nodiscard]]
    bool help() const noexcept
    {
        return help_;
    }

    /** Returns true if the application version was requested.
     *
     * @return True if version requested
     */
    [[nodiscard]]
    bool version() const noexcept
    {
        return version_;
    }

    /** Program data information
     *
     * @return Program data information
     */
    [[nodiscard]]
    program_data& program() noexcept
    {
        return p_;
    }

    /** Const overload.
     *
     * @return Program data information
     */
    [[nodiscard]]
    const program_data& program() const noexcept
    {
        return p_;
    }

    /** Returns the logging level.
     *
     * @return Logging level
     */
    [[nodiscard]]
    log::level log_level() const noexcept
    {
        return log_level_;
    }

    /** Overrides the normalised input detection to load the program data as
     *  non-normalised.
     *
     * @return True to force non-normalised
     */
    [[nodiscard]]
    bool force_non_normalised() const noexcept
    {
        return force_nn_;
    }

    /** Returns the debugger script path, or an empty optional if not specified.
     *
     * @return Debugger script path, if specified
     */
    [[nodiscard]]
    const std::optional<std::filesystem::path>& debugger_script() const noexcept
    {
        return debugger_script_;
    }

private:
    bool help_;
    bool version_;
    program_data p_;
    log::level log_level_;
    bool force_nn_;
    std::optional<std::filesystem::path> debugger_script_;
};

/** Prints the program source into @a stream.
 *
 * @param stream Textual output stream
 * @param source Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream,
                         const argument_parser::program_source& source);

/** Prints the help output into @a stream.
 *
 * @param stream Textual output stream
 * @param parser Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, const argument_parser& parser);
}
