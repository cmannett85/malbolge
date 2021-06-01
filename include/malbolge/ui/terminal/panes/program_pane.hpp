/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/ternary.hpp"
#include "malbolge/ui/terminal/panes/functional_pane.hpp"
#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/utility/signal.hpp"

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** The program pane.
 *
 * This provides the user:
 * - An area to create, load, and edit Malbolge programs
 * - Management and display of breakpoints
 * - Controls to start and stop program execution
 *
 * This class can be moved, but not copied.
 */
class program_pane : public functional_pane
{
public:
    /** Signals a program to be started.
     *
     * @tparam std::string_view Program code, unedited from what is in the UI
     */
    using program_signal_type = utility::signal<std::string_view>;

    /** Signals that the currently running program must stop.
     */
    using stop_signal_type = utility::signal<>;

    /** Carries input data for the currently running program.
     *
     * @tparam std::string Input data
     */
    using input_signal_type = utility::signal<std::string>;

    /** Signals that a breakpoint should be added/updated.
     *
     * @tparam math::ternary Address to add the breakpoint
     * @tparam std::size_t Ignore count
     */
    using add_breakpoint_signal_type = utility::signal<math::ternary, std::size_t>;

    /** Signals that breakpoint should be removed at the given address.
     *
     * @tparam math::ternary Address
     */
    using remove_breakpoint_signal_type = utility::signal<math::ternary>;

    /** Constructor.
     *
     * @param pdata Program source information from command line arguments
     * @exception system_exception Thrown if the program is being from disk and
     * cannot be read
     */
    explicit program_pane(argument_parser::program_data pdata);

    /** Destructor.
     */
    virtual ~program_pane() override = default;

    [[nodiscard]]
    virtual std::string_view name() const noexcept override;

    [[nodiscard]]
    virtual const key_list_type& key_bindings() const noexcept override;

    virtual void key_press(int key) override;

    /** Notify the UI that the program has stopped.
     *
     * This is a no-op if the UI already regards the program as stopped.
     */
    void program_stopped() noexcept;

    /** Notify the UI that a breakpoint has hit at @a address.
     *
     * @param address Virtual memory address of the hit breakpoint
     */
    void breakpoint_hit(math::ternary address) noexcept;

    /** Register @a slot to be called when the program signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    program_signal_type::connection
    register_for_state_signal(program_signal_type::slot_type slot)
    {
        return prog_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the stop signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    stop_signal_type::connection
    register_for_stop_signal(stop_signal_type::slot_type slot)
    {
        return stop_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the input signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    input_signal_type::connection
    register_for_input_signal(input_signal_type::slot_type slot)
    {
        return input_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the add breakpoint signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    add_breakpoint_signal_type::connection
    register_for_add_breakpoint_signal(add_breakpoint_signal_type::slot_type slot)
    {
        return add_bp_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the remove breakpoint signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    remove_breakpoint_signal_type::connection
    register_for_remove_breakpoint_signal(remove_breakpoint_signal_type::slot_type slot)
    {
        return remove_bp_sig_.connect(std::move(slot));
    }

protected:
    virtual void draw() override;

private:
    std::string prog_;
    std::unordered_map<math::ternary, std::size_t> bps_;

    program_signal_type prog_sig_;
    stop_signal_type stop_sig_;
    input_signal_type input_sig_;
    add_breakpoint_signal_type add_bp_sig_;
    remove_breakpoint_signal_type remove_bp_sig_;
};
}
}
}
