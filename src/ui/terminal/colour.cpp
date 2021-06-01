/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"
#include "malbolge/ui/terminal/colour.hpp"
#include "malbolge/traits.hpp"

#include <boost/core/ignore_unused.hpp>

using namespace malbolge;

ui::terminal::colour_state::colour_state(WINDOW* win,
                                         colour foreground,
                                         colour background) :
    win_{win}
{
    if (!win) {
        throw basic_exception{"Null window during colour assignment"};
    }

    if (!has_colors()) {
        return;
    }

    val_ = encode_colour(foreground, background);
    init_pair(val_,
              traits::to_underlying_type(foreground),
              traits::to_underlying_type(background));

    wattron(win_, COLOR_PAIR(val_));
}

ui::terminal::colour_state::~colour_state()
{
    reset();
}

void ui::terminal::colour_state::reset()
{
    if (!has_colors()) {
        return;
    }

    if (win_) {
        wattroff(win_, COLOR_PAIR(val_));
        win_ = nullptr;
    }
}

ui::terminal::colour_state::encoded_colour_type
ui::terminal::colour_state::encode_colour(colour fg, colour bg) noexcept
{
    static constexpr auto fg_shift = 8u;

    return (traits::to_underlying_type(fg) << fg_shift) |
           traits::to_underlying_type(bg);
}
