/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/ui/terminal/core.hpp"
#include "malbolge/ui/terminal/layout.hpp"
#include "malbolge/ui/terminal/panes/command_pane.hpp"
#include "malbolge/ui/terminal/panes/log_pane.hpp"
#include "malbolge/ui/terminal/panes/output_pane.hpp"
#include "malbolge/ui/terminal/panes/program_pane.hpp"
#include "malbolge/utility/tuple_iterator.hpp"

#include <ncurses.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>

using namespace malbolge;
using namespace ui;

struct terminal::core::impl_t
{
    explicit impl_t() :
        guard{ctx.get_executor()}
    {
        initscr();
        layout = std::make_unique<terminal::layout>(LINES, COLS);
    }

    ~impl_t()
    {
        layout.reset();
        endwin();
    }

    std::unique_ptr<terminal::layout> layout;
    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<decltype(ctx)::executor_type> guard;
};

terminal::core::core(argument_parser::program_data program_data) :
    impl_{std::make_shared<impl_t>()}
{
    // Build the layout and wire the connections between the components
    auto cmd_p = std::make_shared<terminal::command_pane>(impl_->ctx);
    auto prog_p = std::make_shared<terminal::program_pane>(std::move(program_data));
    auto out_p = std::make_shared<terminal::output_pane>();
    auto log_p = std::make_shared<terminal::log_pane>(impl_->ctx);

    impl_->layout->replace(prog_p);
    impl_->layout->split(cmd_p,
                         terminal::layout::split_direction::HORIZONTAL,
                         prog_p);
    impl_->layout->split(out_p,
                         terminal::layout::split_direction::VERTICAL,
                         prog_p);
    impl_->layout->split(log_p,
                         terminal::layout::split_direction::HORIZONTAL,
                         out_p);

    utility::tuple_iterator([&](auto, auto& p) { cmd_p->register_pane(std::move(p)); },
                            prog_p, out_p, log_p);

    impl_->layout->refresh();
}

void terminal::core::run()
{
    impl_->layout->refresh();
    impl_->ctx.run();
}
