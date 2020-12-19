/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/virtual_memory.hpp"
#include "malbolge/utility/gate.hpp"
#include "malbolge/utility/mutex_wrapper.hpp"
#include "malbolge/debugger/client_control.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <future>

namespace malbolge
{
/** Represents a virtual CPU.
 *
 * This class can not be copied, but can be moved.
 */
class virtual_cpu
{
public:
    /** Callback type used to notify the debugger when the program has started
     * and finished.
     * @param started True if the program has started (i.e. READY), false if it
     * has finished (i.e. STOPPED)
     */
    using running_callback_type = std::function<void (bool started)>;

    /** Callback type used to provide the debugger with execution step data.
     */
    using step_data_callback_type = debugger::client_control::breakpoint::callback_type;

    /** Execution state.
     */
    enum class execution_state {
        READY,      ///< Ready to run
        RUNNING,    ///< Program running
        STOPPED,    ///< Program stopped
        NUM_STATES  ///< Number of execution states
    };

    /** Constructor.
     *
     * @param vmem Virtual memory containing the initialised memory space
     * (including program data)
     */
    explicit virtual_cpu(virtual_memory vmem);

    /** Destructor.
     */
    ~virtual_cpu();

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
    virtual_cpu& operator=(virtual_cpu&& other);

    virtual_cpu(const virtual_cpu&) = delete;
    virtual_cpu& operator=(const virtual_cpu&) = delete;

    /** Returns the vCPU execution state.
     *
     * @return Execution state
     */
    execution_state state() const
    {
        return *state_;
    }

    /** Execute the program asynchronously.
     *
     * @param istr Input stream
     * @param ostr Output stream
     * @param mtx Mutex for accessing the streams.  Only required if at least
     * one stream is not cin/cout
     * @return Future holding nothing, or an exception
     * @exception execution_exception Thrown if the program has already ran, or
     * cin/cout is used as a stream but mtx is not valid
     */
    std::future<void> run(std::istream& istr = std::cin,
                          std::ostream& ostr = std::cout,
                          utility::mutex_wrapper mtx = {});

    /** Overload.
     *
     * Execute the program asynchronously, but uses callbacks instead of a
     * future.
     * @note The callbacks are called from the execution thread
     * @param stopped Callback called when execution has stopped.  Its
     * argument will be nullptr if the program executed successfully
     * @param waiting_for_input Callback called when execution is paused
     * waiting for user input
     * @param istr Input stream
     * @param ostr Output stream
     * @param mtx Mutex for accessing the streams.  Only required if at least
     * one stream is not cin/cout
     * @exception execution_exception Thrown if the program has already ran, or
     * cin/cout is used as a stream but mtx is not valid
     */
    void run(std::function<void (std::exception_ptr)> stopped,
             std::function<void ()> waiting_for_input,
             std::istream& istr = std::cin,
             std::ostream& ostr = std::cout,
             utility::mutex_wrapper mtx = {});

    /** Asynchronously stops execution.
     *
     * This is ignored if a program is not running.
     */
    void stop();

    /** Configure a debugger to gain control of the execution and view memory.
     *
     * @param running Callback to notify the debugger that the program has
     * started or finished
     * @param step_data Callback to notify the debugger of the current memory
     * access and which register (C or D) initiated the access
     * @return A configured vCPU controller
     * @exception basic_exception Thrown if the program is already running,
     * or a debugger has already been configured
     */
    debugger::vcpu_control
    configure_debugger(running_callback_type running,
                       step_data_callback_type step_data);

    /** Applies a delay between instruction cycles, useful for
     * debugging/tracing.
     *
     * As expected, a zero value duration disables the delay.
     *
     * Despite the template params, this method is usually called with the
     * Chrono literals, e.g.:
     * @code
     * vcpu.set_cycle_delay(100ms);
     * @endcode
     * @tparam Rep Number of ticks
     * @tparam Period Tick period ratio (i.e. the number of seconds per tick)
     * @param delay Delay duration
     */
    template <class Rep, class Period>
    void set_cycle_delay(const std::chrono::duration<Rep, Period>& delay)
    {
        *cycle_delay_ = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
    }

private:
    struct debugger_data
    {
        running_callback_type running_cb;
        step_data_callback_type step_data_cb;

        std::mutex mtx;
        debugger::vcpu_control::callback_type stop_cb;
        debugger::vcpu_control::callback_type resume_cb;

        // Populated once the worker thread starts
        decltype(debugger::vcpu_control::address_value) address_value;
        decltype(debugger::vcpu_control::register_value) register_value;

        utility::gate gate;
    };

    void basic_run_check(std::istream& istr,
                         std::ostream& ostr,
                         utility::mutex_wrapper mtx);

    static void vcpu_loop(virtual_memory& vmem,
                          std::atomic<virtual_cpu::execution_state>& state,
                          std::function<void ()> waiting_for_input,
                          std::istream& istr,
                          std::ostream& ostr,
                          utility::mutex_wrapper mtx,
                          std::shared_ptr<debugger_data> debugger,
                          std::shared_ptr<std::atomic_uint> cycle_delay);

    std::thread thread_;
    std::shared_ptr<std::atomic<execution_state>> state_;
    virtual_memory vmem_;

    std::shared_ptr<std::atomic_uint> cycle_delay_;
    std::shared_ptr<debugger_data> debugger_;
};

/** Textual streaming operator.
 *
 * @param stream Textual output stream
 * @param state vCPU execution state
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream,
                         virtual_cpu::execution_state state);
}
