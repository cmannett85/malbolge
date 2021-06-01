/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/ui/terminal/panes/functional_pane.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** The log pane.
 *
 * This provides the user:
 * - An area to view the log output
 * - An option to save the output to disk
 * - An option to clear the output
 * - An option to set the log level
 */
class log_pane : public functional_pane
{
public:
    /** Default constructor.
     *
     * @param ctx Event loop
     */
    explicit log_pane(boost::asio::io_context& ctx);

    /** Destructor.
     */
    virtual ~log_pane() override = default;

    [[nodiscard]]
    virtual std::string_view name() const noexcept override;

    [[nodiscard]]
    virtual const key_list_type& key_bindings() const noexcept override;

    virtual void key_press(int key) override;

protected:
     virtual void draw() override;

private:
    void update_name() noexcept;

    void read() noexcept;
    void read(const boost::system::error_code& ec) noexcept;

    void save();

    std::reference_wrapper<boost::asio::io_context> ctx_;
    boost::asio::steady_timer read_timer_;

    std::stringstream stream_;
    std::string output_;

    mutable std::string name_;
};
}
}
}
