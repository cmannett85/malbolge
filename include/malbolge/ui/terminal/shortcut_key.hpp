/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include <string_view>
#include <vector>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Shortcut keys available.
 *
 * This list obviously isn't exhaustive, it should be added to when a new
 * functional_pane needs a shortcut key that isn't already listed.
 *
 * @note The underlying values can change if the underlying UI library changes
 */
enum class shortcut_key : int
{
    CTRL_SPACE  = 0,    ///< Ctrl+Space
    CTRL_A      = 1,    ///< Ctrl+a
    CTRL_B      = 2,    ///< Ctrl+b
    CTRL_C      = 3,    ///< Ctrl+c
    CTRL_D      = 4,    ///< Ctrl+d
    CTRL_Q      = 17,   ///< Ctrl+q
    CTRL_R      = 18,   ///< Ctrl+r
    CTRL_S      = 19,   ///< Ctrl+s
    CTRL_DELETE = 520,  ///< Ctrl+Delete
};

/** Returns a string representation of @a key.
 *
 * This is suitable for display within the UI.
 * @param key Key to convert
 * @return String representation, or "Unknown" if not handled
 */
[[nodiscard]]
std::string_view to_string(shortcut_key key) noexcept;
}
}
}
