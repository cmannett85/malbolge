/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/utility/signal.hpp"
#include "malbolge/virtual_memory.hpp"

namespace malbolge
{
/** Represents a virtual CPU.
 *
 * This class can not be copied, but can be moved.
 */
class virtual_cpu
{
public:
    /** Execution state.
     */
    enum class execution_state {
        READY,              ///< Ready to run
        RUNNING,            ///< Program running
        PAUSED,             ///< Program paused
        WAITING_FOR_INPUT,  ///< Similar to paused, except the program will
                            ///< resume when input data provided
        STOPPED,            ///< Program stopped, cannot be resumed or ran again
        NUM_STATES          ///< Number of execution states
    };

    /** vCPU register identifiers.
     */
    enum class vcpu_register {
        A,              ///< Accumulator
        C,              ///< Code pointer
        D,              ///< Data pointer
        NUM_REGISTERS   ///< Number of registers
    };

    /** Signal type to indicate the program running state, and any exception in
     *  case of error.
     *
     * In case of an error the execution state is always STOPPED.
     * @tparam execution_state Execution state
     * @tparam std::exception_ptr Exception pointer, null if no error
     */
    using state_signal_type = utility::signal<execution_state, std::exception_ptr>;

    /** Signal type carrying program output data.
     *
     * @tparam char Character output from program
     */
    using output_signal_type = utility::signal<char>;

    /** Signal type fired when a breakpoint is hit.
     *
     * @tparam math::ternary Address the breakpoint resides at
     */
    using breakpoint_hit_signal_type = utility::signal<math::ternary>;

    /** Address value result callback type.
     *
     * @param address Address in virtual memory that was requested
     * @param value Value at @a address
     */
    using address_value_callback_type =
        std::function<void (math::ternary address,
                            math::ternary value)>;

    /** Register value result callback type.
     *
     * @param reg Requested register
     * @param address If @a reg is C or D then it contains an address
     * @param value The value of the register if @a address is empty, otherwise
     * the value at @a address
     */
    using register_value_callback_type =
        std::function<void (vcpu_register reg,
                            std::optional<math::ternary> address,
                            math::ternary value)>;

    /** Constructor.
     *
     * Although it is not emitted in the state signal, the instance begins in
     * a execution_state::READY state.
     * @param vmem Virtual memory containing the initialised memory space
     * (including program data)
     */
    explicit virtual_cpu(virtual_memory vmem);

    /** Move constructor.
     *
     * @param other Instance to move from
     */
    virtual_cpu(virtual_cpu&& other) = default;

    /** Move assignment operator.
     *
     * @param other Instance to move from
     * @return A reference to this
     */
    virtual_cpu& operator=(virtual_cpu&& other) = default;

    virtual_cpu(const virtual_cpu&) = delete;
    virtual_cpu& operator=(const virtual_cpu&) = delete;

    /** Destructor.
     */
    ~virtual_cpu();

    /** Runs or resumes program execution.
     *
     * If the program is already running or waiting-for-input, then this is a
     * no-op.
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     * @exception execution_exception Thrown if the vCPU ha already been
     * stopped
     */
    void run();

    /** Pauses a running program.
     *
     * If the program is already paused or waiting-for-input, then this is a
     * no-op.
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     * @exception execution_exception Thrown if the vCPU ha already been
     * stopped
     */
    void pause();

    /** Advances the program by a single execution.
     *
     * If the program is running, this will pause it and then advance by a
     * single execution.  If the program is waiting-for-input, then this is a
     * no-op.
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     * @exception execution_exception Thrown if the vCPU ha already been
     * stopped
     */
    void step();

    /** Adds @a data to the input queue for the program.
     *
     * If the program is waiting for input, then calling this will resume
     * program execution.
     * @param data Input add to add
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    void add_input(std::string data);

    /** Adds a breakpoint to the program.
     *
     * If another breakpoint is already at the given address, it is replaced.
     * Breakpoints can be added to the program whilst in any non-STOPPED state,
     * but results can be unpredicatable if the program is running.
     *
     * The breakpoint-hit signal is fired when a breakpoint is reached.  The
     * signal is fired when then code pointer reaches the address i.e.
     * @em before the instruction is executed, so you need to step to see the
     * result of the instruction execution.
     * @param address Address to attach a breakpoint
     * @param ignore_count Number of times the breakpoint is hit before the
     * breakpoint_hit_signal_type signal is fired
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    void add_breakpoint(math::ternary address, std::size_t ignore_count = 0);

    /** Removes a breakpoint at the given address.
     *
     * This is a no-op if there is no breakpoint at @a address.
     * @param address vmem address to remove a breakpoint from
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    void remove_breakpoint(math::ternary address);

    /** Asynchronously returns the value at a given vmem address via @a cb.
     *
     * @param address vmem address
     * @param cb Called with the result
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    void address_value(math::ternary address,
                       address_value_callback_type cb) const;

    /** Asynchronously returns the address and/or value of a given register.
     *
     * @param reg Register to query
     * @param cb For C and D registers returns the address held in the register
     * and the value it points at, for the A register it only returns the value
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    void register_value(vcpu_register reg, register_value_callback_type cb) const;

    /** Register @a slot to be called when the state signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    state_signal_type::connection
    register_for_state_signal(state_signal_type::slot_type slot);

    /** Register @a slot to be called when the output signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    output_signal_type::connection
    register_for_output_signal(output_signal_type::slot_type slot);

    /** Register @a slot to be called when the breakpoint hit signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     * @exception execution_exception Thrown if backend has been destroyed,
     * usually as a result of use-after-move
     */
    breakpoint_hit_signal_type::connection
    register_for_breakpoint_hit_signal(breakpoint_hit_signal_type::slot_type slot);

private:
    void impl_check() const;

    class impl_t;
    std::shared_ptr<impl_t> impl_;
};

/** Textual streaming operator for virtual_cpu::vcpu_register.
 *
 * @param stream Output stream
 * @param register_id Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, virtual_cpu::vcpu_register register_id);

/** Textual streaming operator for virtual_cpu::execution_state.
 *
 * @param stream Output stream
 * @param state Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, virtual_cpu::execution_state state);
}
