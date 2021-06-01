/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/ui/terminal/panes/output_pane.hpp"

using namespace malbolge;

namespace
{
constexpr auto INIT_BUFFER_SIZE = 1024u;
}

ui::terminal::output_pane::output_pane()
{
    // Reserve some initial memory for the output to minimise early reallocation
    output_.reserve(INIT_BUFFER_SIZE);
}

std::string_view ui::terminal::output_pane::name() const noexcept
{
    return "Output";
}

void ui::terminal::output_pane::append(char c) noexcept
{
    output_.push_back(c);
    refresh();
}

void ui::terminal::output_pane::draw()
{
    wprintw(window(), "%s", output_.data());
}
