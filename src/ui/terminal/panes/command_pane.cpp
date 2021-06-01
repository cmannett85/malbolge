/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"
#include "malbolge/log.hpp"
#include "malbolge/ui/terminal/colour.hpp"
#include "malbolge/ui/terminal/panes/command_pane.hpp"
#include "malbolge/utility/string_view_ops.hpp"
#include "malbolge/version.hpp"

#include <boost/asio/post.hpp>

#include <algorithm>

using namespace malbolge;
using namespace malbolge::utility::string_view_ops;
using namespace std::string_literals;

namespace
{
const auto name_and_project_version = "Malbolge v"s + project_version;

constexpr auto max_height = 3u;
constexpr auto border_width = std::size_t{1};

constexpr auto read_timeout = 1;    // Tenths of a second
constexpr auto cycle_active_key = ui::terminal::shortcut_key::CTRL_Q;

constexpr auto init_pane_reservation = 5;
}

ui::terminal::command_pane::command_pane(boost::asio::io_context& ctx) :
    ctx_{ctx}
{
    start_color();
    noecho();
    raw();
    keypad(stdscr, true);
    halfdelay(read_timeout);

    // Just to prevent a bunch of reallocations at during startup
    key_mapping_.reserve(init_pane_reservation);
}

ui::terminal::command_pane::~command_pane()
{
    echo();
    nocbreak();
    keypad(stdscr, false);
}

auto ui::terminal::command_pane::maximum_size() const noexcept -> coordinate
{
    return {max_height, std::numeric_limits<coordinate::col_type>::max()};
}

std::string_view ui::terminal::command_pane::name() const noexcept
{
    return name_and_project_version;
}

void ui::terminal::command_pane::refresh()
{
    if (!win_) [[unlikely]] {
        throw basic_exception{"Window not configured for drawing"};
    }

    curs_set(0);
    wclear(win_.get());

    // Draw the title and border
    box(win_.get(), 0, 0);
    mvwprintw(win_.get(), border_width, border_width, "%s\t", name().data());

    // Draw the bindings
    if (active_) {
        for (const auto& binding : key_mapping_[*active_]->key_bindings()) {
            {
                auto c = colour_state{win_.get(), colour::BLACK, colour::WHITE};
                wprintw(win_.get(), "%s", to_string(binding.key).data());
            }
            wprintw(win_.get(), " %s  ", binding.description.data());
        }
    }

    wrefresh(win_.get());
}

void ui::terminal::command_pane::register_pane(pane_ptr p)
{
    if (!p) {
        throw basic_exception{"Cannot register a nullptr pane"};
    }

    // Check if any key binding conflicts with this pane's own
    if (std::ranges::any_of(p->key_bindings(), [](auto b) { return b.key == cycle_active_key; })) {
        throw basic_exception{"Key binding conflict with bindings from "s + p->name()};
    }

    if (std::ranges::any_of(key_mapping_, [&](const auto& existing_p) { return existing_p == p; })) {
        throw basic_exception{p->name() + " has already been registered"s};
    }

    key_mapping_.emplace_back(std::move(p));
    if (!active_) {
        active_ = 0;
        key_mapping_[*active_]->set_active(true);

        read_key();
    }
}

void ui::terminal::command_pane::dimensions_updated()
{
    auto size = pane::size();
    auto pos = pane::position();

    // Unlike the functional_pane equivalent, there's no derived types to
    // worry about and the layout is simple, so there's no need to make any
    // subwindows
    win_ = {
        newwin(size.lines, size.cols, pos.lines, pos.cols),
        [](auto win) { delwin(win); }
    };
    if (!win_) {
        throw basic_exception{"Unable to create/resize pane"};
    }

    touchwin(win_.get());
}

void ui::terminal::command_pane::read_key()
{
    const auto key = getch();
    if (key != ERR) {
        new_key_press(key);
    }

    boost::asio::post(ctx_.get(), [w_ptr = weak_from_this()]() {
        auto ptr =  w_ptr.lock();
        if (!ptr) {
            return;
        }

        ptr->read_key();
    });
}

void ui::terminal::command_pane::new_key_press(int key) noexcept
{
    // The owning thread isn't started until active_ has content, so no need to
    // check it here
    const auto idx = *active_;
    if (key == traits::to_underlying_type(cycle_active_key) &&
        key_mapping_.size() > 1) {
        key_mapping_[idx]->set_active(false, true);

        *active_ = (idx+1) == key_mapping_.size() ? 0u : idx+1;
        key_mapping_[*active_]->set_active(true, true);

        log::print(log::INFO, "Active win: ", key_mapping_[*active_]->name());
        refresh();
        return;
    }

    try {
        key_mapping_[idx]->key_press(key);
    } catch (basic_exception& e) {
        log::print(log::ERROR, "Error processing key \"", std::to_string(key),
                               "\": ", e.what());
    }
}
