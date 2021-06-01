/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/utility/argument_parser.hpp"

namespace malbolge
{
/** Namespace for all GUI types and functions.
 */
namespace ui
{
/** Namespace for an interactive terminal GUI, based on ncurses.
 */
namespace terminal
{
/** Core terminal functionality.
 *
 *  This class can be moved, but not copied.
 */
class core
{
public:
    /** Constructor.
     *
     * @param program_data Program data
     */
    explicit core(argument_parser::program_data program_data);

    core(const core&) = delete;
    core& operator=(const core&) = delete;
    core(core&&) noexcept = default;
    core& operator=(core&&) noexcept = delete;

    /** Runs the interactive terminal UI.
     *
     * This function blocks until the UI is exited, which should signal the
     * application to exit.
     * @exception basic_exception Thrown if called on a moved-from instance
     */
    void run();

private:
    // We use PIMPL to minimise the amount of TUs that import the ncurses header
    struct impl_t;
    std::shared_ptr<impl_t> impl_;
};
}
}
}
