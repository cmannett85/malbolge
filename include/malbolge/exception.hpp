/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/ternary.hpp"

#include <optional>

namespace malbolge
{
/** Malbolge exception base class.
 *
 * All Malbolge exceptions derive from the same base class, for polymorphic
 * reasons.  We use an alias, rather than the type directly, so it can be
 * swapped out for a custom type in future very easily.
 */
using basic_exception = std::runtime_error;

/** Represents a location in a Malbolge source file.
 *
 * To make it more readable for users, the members should begin at 1.
 */
struct source_location
{
    /** Constructor.
     *
     * @param l Line number
     * @param c Column number
     */
    source_location(math::ternary::underlying_type l = 1,
                    math::ternary::underlying_type c = 1) :
        line{l},
        column{c}
    {}

    math::ternary::underlying_type line;    ///< Line number
    math::ternary::underlying_type column;  ///< Column number
};

/** An optional source_location alias.
 */
using optional_source_location = std::optional<source_location>;

/** String conversion for optional_source_location.
 *
 * If @a loc is empty, the returned string is empty.
 * @param loc Source location
 * @return String equivalent to it
 */
std::string to_string(const optional_source_location& loc);

/** Textual streaming operator for optional_source_location.
 *
 * @param stream Output stream
 * @param loc Optional stream location instance
 * @return @a stream
 */
inline std::ostream& operator<<(std::ostream& stream,
                                const optional_source_location& loc)
{
    return stream << to_string(loc);
}

/** Exception thrown by the parser.
 *
 * This is also used during file loading from disk.
 */
class parse_exception : public basic_exception
{
public:
    /** Constructor.
     *
     * @param msg Message
     * @param loc Source location of the error
     */
    explicit parse_exception(const std::string& msg,
                             optional_source_location loc = {});

    /** Destructor.
     */
    virtual ~parse_exception() = default;

    /** Returns true if a location has been set.
     *
     * @return Location set
     */
    bool has_location() const noexcept
    {
        return !!loc_;
    }

    /** Returns the location.
     *
     * @return The location, may be empty
     */
    optional_source_location location() const noexcept
    {
        return loc_;
    }

private:
    optional_source_location loc_;
};

/** Execution thrown during program execution.
 */
class execution_exception : public basic_exception
{
public:
    /** Constructor.
     *
     * @param msg Message
     * @param execution_step Instruction execution step, may aid in debugging
     */
    explicit execution_exception(const std::string& msg,
                                 std::size_t execution_step);

    /** Destructor.
     */
    virtual ~execution_exception() = default;

    /** Returns the instruction execution step.
     *
     * @return Execution step
     */
    std::size_t step() const noexcept
    {
        return step_;
    }

private:
    std::size_t step_;
};

/** Execution thrown during virtual machine operation, not relating to Malbolge
 * program execution.
 */
class system_exception : public basic_exception
{
public:
    /** Constructor.
     *
     * The error code is used with std::system_category() to generate a
     * std::error_code.
     * @param msg Message
     * @param error_code System error code
     */
    explicit system_exception(const std::string& msg, int error_code);

    /** Destructor.
     */
    virtual ~system_exception() = default;

    /** Returns the error code.
     *
     * @return Error code
     */
    const std::error_code& code() const noexcept
    {
        return code_;
    }

private:
    std::error_code code_;
};
}
