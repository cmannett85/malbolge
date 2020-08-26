/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/virtual_memory.hpp"
#include "malbolge/utility/mutex_wrapper.hpp"

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
    explicit virtual_cpu(virtual_memory vmem) :
        state_{std::make_shared<decltype(state_)::element_type>(execution_state::READY)},
        vmem_(std::move(vmem))
    {}

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
     * @throw execution_exception Thrown if the program has already ran, or
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
     * @throw execution_exception Thrown if the program has already ran, or
     * cin/cout is used as a stream but mtx is not valid
     */
    void run(std::function<void (std::exception_ptr)> stopped,
             std::function<void ()> waiting_for_input,
             std::istream& istr = std::cin,
             std::ostream& ostr = std::cout,
             utility::mutex_wrapper mtx = {});

    /** Asynchronously stops execution.
     */
    void stop();

private:
    void basic_run_check(std::istream& istr,
                         std::ostream& ostr,
                         utility::mutex_wrapper mtx);

    std::thread thread_;
    std::shared_ptr<std::atomic<execution_state>> state_;
    virtual_memory vmem_;
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
