/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/log.hpp"
#include "malbolge/utility/string_view_ops.hpp"
#include "malbolge/traits.hpp"
#include "malbolge/utility/stream_helpers.hpp"
#include "malbolge/ui/terminal/panes/log_pane.hpp"

using namespace malbolge;
using namespace utility::string_view_ops;
using namespace std::chrono_literals;
using namespace std::string_literals;

namespace
{
constexpr auto INIT_BUFFER_SIZE = 4096;
constexpr auto POLL_INTERVAL = 20ms;
}

ui::terminal::log_pane::log_pane(boost::asio::io_context& ctx) :
    ctx_{ctx},
    read_timer_{ctx}
{
    // Replace the logging output stream with our own
    log::set_log_stream(stream_);

    // Increase the default log level
    log::set_log_level(log::DEBUG);
    update_name();

    // Reserve some initial memory for the output to minimise early reallocation
    output_.reserve(INIT_BUFFER_SIZE);

    // Push the initial read onto the loop
    read();
}

std::string_view ui::terminal::log_pane::name() const noexcept
{
    return name_;
}

const ui::terminal::functional_pane::key_list_type&
ui::terminal::log_pane::key_bindings() const noexcept
{
    static const auto bindings = key_list_type{
        {shortcut_key::CTRL_R,  "Clear"},
        {shortcut_key::CTRL_S,  "Save"},
        {shortcut_key::CTRL_D,  "Cycle debug level"}
    };
    return bindings;
}

void ui::terminal::log_pane::key_press(int key)
{
    switch (key) {
    case traits::to_underlying_type(shortcut_key::CTRL_R):
        output_.clear();
        refresh();
        break;
    case traits::to_underlying_type(shortcut_key::CTRL_S):
        save();
        break;
    case traits::to_underlying_type(shortcut_key::CTRL_D):
    {
        auto lvl = log::log_level();
        log::set_log_level(lvl == log::VERBOSE_DEBUG ? log::ERROR :
                                                       static_cast<log::level>(lvl-1));
        update_name();
        refresh();
        break;
    }
    default:
        break;
    }
}

void ui::terminal::log_pane::draw()
{
    wclear(window());
    curs_set(0);
    wprintw(window(), output_.data());
}

void ui::terminal::log_pane::update_name() noexcept
{
    name_ = "Log ("s + to_string(log::log_level()) + ")";
}

void ui::terminal::log_pane::read() noexcept
{
    read_timer_.expires_after(POLL_INTERVAL);
    read_timer_.async_wait([this](const auto& ec) {
        read(ec);
    });
}

void ui::terminal::log_pane::save()
{

}

void ui::terminal::log_pane::read(const boost::system::error_code& ec) noexcept
{
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            log::print(log::ERROR, "Log output read error: ", ec.message());
        }
        return;
    }

    auto needs_refresh = false;
    for (auto tmp = ""s; std::getline(stream_, tmp); ) {
        output_.append(tmp);
        needs_refresh = true;
    }

    if (needs_refresh) {
        refresh();
    }

    // Schedule the next check
    read();
}
