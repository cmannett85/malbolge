/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/utility/file_load.hpp"
#include "malbolge/utility/stream_helpers.hpp"
#include "malbolge/ui/terminal/panes/program_pane.hpp"

using namespace malbolge;
using namespace std::string_literals;

namespace
{
[[nodiscard]]
std::string load_program(argument_parser::program_data pdata)
{
    if (pdata.source == argument_parser::program_source::DISK) {
        // Load the file off disk
        return utility::file_load<std::string>(pdata.data);
    } else if (pdata.source == argument_parser::program_source::STRING) {
        return std::move(pdata.data);
    }

    // Load from cin, but don't wait if there's nothing immediately available
    if (utility::data_available(std::cin)) {
        auto data = ""s;
        for (auto line = ""s; std::getline(std::cin, line); ) {
            data += line;
        }

        return data;
    }

    return {};
}
}

ui::terminal::program_pane::program_pane(argument_parser::program_data initial_pdata) :
    prog_{load_program(std::move(initial_pdata))}
{}

std::string_view ui::terminal::program_pane::name() const noexcept
{
    return "Program";
}

const ui::terminal::functional_pane::key_list_type&
ui::terminal::program_pane::key_bindings() const noexcept
{
    static const auto bindings = key_list_type{
        {shortcut_key::CTRL_SPACE,  "Play/pause"},
        {shortcut_key::CTRL_C,      "Stop/exit"},
        {shortcut_key::CTRL_B,      "Ins. BP"},
        {shortcut_key::CTRL_DELETE, "Del. BP"},
    };
    return bindings;
}

void ui::terminal::program_pane::key_press(int key)
{
    switch (key) {
    case traits::to_underlying_type(shortcut_key::CTRL_SPACE):
        prog_sig_(prog_);
        break;
    case traits::to_underlying_type(shortcut_key::CTRL_C):
        stop_sig_();
        break;
    case traits::to_underlying_type(shortcut_key::CTRL_B):
    case traits::to_underlying_type(shortcut_key::CTRL_DELETE):
        // To do
        break;
    default:
        // To do
        return;
    }
}

void ui::terminal::program_pane::program_stopped() noexcept
{

}

void ui::terminal::program_pane::breakpoint_hit(math::ternary /*address*/) noexcept
{

}

void ui::terminal::program_pane::draw()
{
    werase(window());
    mvwprintw(window(), 0, 0, "%s", prog_.data());
}
