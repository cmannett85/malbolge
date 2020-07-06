/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/virtual_memory.hpp"

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
     * @return Future holding the result of the program, or an exception from
     * it
     * @throw execution_exception Thrown if there is logic error within the
     * program, or if the program has already ran
     */
    std::future<void> run(std::istream& istr = std::cin,
                          std::ostream& ostr = std::cout);

    /** Stops execution.
     */
    void stop();

private:
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
