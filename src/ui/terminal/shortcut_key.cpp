/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/ui/terminal/shortcut_key.hpp"

using namespace malbolge;

std::string_view ui::terminal::to_string(shortcut_key key) noexcept
{
    switch (key) {
    case shortcut_key::CTRL_SPACE:    return "Ctrl+Space";
    case shortcut_key::CTRL_A:        return "Ctrl+a";
    case shortcut_key::CTRL_B:        return "Ctrl+b";
    case shortcut_key::CTRL_C:        return "Ctrl+c";
    case shortcut_key::CTRL_D:        return "Ctrl+d";
    case shortcut_key::CTRL_Q:        return "Ctrl+q";
    case shortcut_key::CTRL_R:        return "Ctrl+r";
    case shortcut_key::CTRL_S:        return "Ctrl+s";
    case shortcut_key::CTRL_DELETE:   return "Ctrl+Delete";
    default:
        return "Unknown";
    }
}
