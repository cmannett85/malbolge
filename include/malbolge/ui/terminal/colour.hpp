/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include <cstdint>
#include <unordered_set>

#include <ncurses.h>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Built-in terminal colours.
 *
 * @note The underlying values can change if the underlying UI library changes
 */
enum class colour : std::uint8_t
{
    BLACK,      ///< Black
    RED,        ///< Red
    GREEN,      ///< Green
    YELLOW,     ///< Yellow
    BLUE,       ///< Blue
    MAGENTA,    ///< Magenta
    CYAN,       ///< Cyan
    WHITE       ///< White
};

/** Sets the foreground and background colours whilst the object is alive.
 *
 * Sets the colours back to the default upon destruction.  If the terminal does
 * not support colours, all operations are no-ops.
 *
 * This class can be copied and moved.
 */
class colour_state
{
public:
    /** Constructor.
     *
     * @param win ncurses window to apply the colour to
     * @param foreground Foreground colour
     * @param background Background colour
     * @exception basic_exception Thrown if @a win is nullptr
     */
    explicit colour_state(WINDOW* win,
                          colour foreground,
                          colour background = colour::BLACK);

    /** Destructor.
     *
     * Sets the terminal colours back to the default values.
     */
    ~colour_state();

    /** Sets the terminal colours back to the default values.
     *
     * Equivalent to the destructor.
     */
    void reset();

private:
    using encoded_colour_type = std::uint16_t;

    [[nodiscard]]
    static encoded_colour_type encode_colour(colour fg, colour bg) noexcept;

    WINDOW* win_;
    encoded_colour_type val_;
};
}
}
}
