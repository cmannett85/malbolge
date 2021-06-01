/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"
#include "malbolge/ui/terminal/panes/functional_pane.hpp"

using namespace malbolge;

namespace
{
constexpr auto title_height = std::size_t{2};
constexpr auto border_width = std::size_t{1};
}

void ui::terminal::functional_pane::refresh()
{
    if (!win_) [[unlikely]] {
        throw basic_exception{"Window not configured for drawing"};
    }

    wclear(title_win_.get());

    // Draw the title and border
    box(win_.get(), 0, 0);
    wattron(title_win_.get(), active_ ? A_BOLD : A_NORMAL);
    mvwprintw(title_win_.get(), 0, 0, name().data());
    wattroff(title_win_.get(), A_BOLD);
    mvwhline(title_win_.get(), 1, 0, 0, size().cols - (2 * border_width));

    // Get the derived type to draw their contents
    draw();

    wrefresh(win_.get());
    wrefresh(title_win_.get());
    wrefresh(pane_win_.get());
}

const ui::terminal::functional_pane::key_list_type&
ui::terminal::functional_pane::key_bindings() const noexcept
{
    static const auto default_bindings = key_list_type{};
    return default_bindings;
}

void ui::terminal::functional_pane::key_press(int)
{
    // Do nothing
}

void ui::terminal::functional_pane::set_active(bool active, bool refresh)
{
    if (active != active_) {
        active_ = active;
        if (refresh) {
            this->refresh();
        }
    }
}

void ui::terminal::functional_pane::dimensions_updated()
{
    auto size = pane::size();
    auto pos = pane::position();

    try {
        // Create primary window.  This contains the border and title windows
        win_ = {
            newwin(size.lines, size.cols, pos.lines, pos.cols),
            [](auto win) { delwin(win); }
        };
        if (!win_) {
            throw basic_exception{"Unable to create/resize pane"};
        }

        // Create title window
        title_win_ = {
            subwin(win_.get(),
                   title_height, size.cols - (2 * border_width),
                   pos.lines + border_width,  pos.cols + border_width),
            [](auto win) { delwin(win); }
        };
        if (!title_win_) {
            throw basic_exception{"Unable to create/resize title pane"};
        }

        // Create subwindow inside the box for derived types to draw in
        size.lines -= (2 * border_width) + title_height;
        size.cols  -= (2 * border_width);
        pos.lines  += border_width + title_height;
        pos.cols   += border_width;

        pane_win_ = {
            subwin(win_.get(), size.lines, size.cols, pos.lines, pos.cols),
            [](auto win) { delwin(win); }
        };
        if (!pane_win_) {
            throw basic_exception{"Unable to create/resize inner pane"};
        }

        touchwin(win_.get());
        touchwin(title_win_.get());
        touchwin(pane_win_.get());
    } catch (basic_exception&) {
        pane_win_.reset();
        title_win_.reset();
        win_.reset();

        throw;
    }
}
