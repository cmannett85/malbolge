/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/ui/terminal/panes/functional_pane.hpp"

#include <ncurses.h>

#include <boost/asio/io_context.hpp>

#include <thread>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Command pane.
 *
 * This provides the user:
 * - Displays what command key bindings are available for the active pane
 * - Listens to all keypresses and sends them to the active pane
 * - Manages which panes are active
 */
class command_pane : public pane, public std::enable_shared_from_this<command_pane>
{
public:
    /** Function pane shared pointer alias.
     */
    using pane_ptr = std::shared_ptr<functional_pane>;

    /** Constructor.
     *
     * @param ctx Core event loop
     */
    explicit command_pane(boost::asio::io_context& ctx);

    /** Destructor.
     */
    virtual ~command_pane() override;

    [[nodiscard]]
    virtual coordinate maximum_size() const noexcept override;

    [[nodiscard]]
    virtual std::string_view name() const noexcept override;

    virtual void refresh() override;

    /** Registers the pane for management.
     *
     * This allows the pane's active state to managed and keypress notification.
     * The first pane to be registered becomes the active one.  Panes are
     * cycled through in registration order.
     * @param p Pane to register
     * @throw basic_exception Thrown if @p is a nullptr, or has already been
     * registered
     * @throw basic_exception Thrown if a binding in @a p conflicts with the one
     * from the command pane itself
     */
    void register_pane(pane_ptr p);

protected:
    virtual void dimensions_updated() override;

private:
    using window_ptr = std::unique_ptr<WINDOW, std::function<void (WINDOW*)>>;
    using pane_list = std::vector<pane_ptr>;

    void read_key();
    void new_key_press(int key) noexcept;

    window_ptr win_;
    pane_list key_mapping_;
    std::optional<std::size_t> active_;

    std::reference_wrapper<boost::asio::io_context> ctx_;
};
}
}
}
