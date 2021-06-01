/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/ui/terminal/pane.hpp"

using namespace malbolge;

void ui::terminal::pane::set_dimensions(coordinate size, coordinate pos)
{
    size_ = size;
    pos_ = pos;

    dimensions_updated();
}

auto ui::terminal::pane::maximum_size() const noexcept -> coordinate
{
    return {std::numeric_limits<coordinate::line_type>::max(),
            std::numeric_limits<coordinate::col_type>::max()};
}
