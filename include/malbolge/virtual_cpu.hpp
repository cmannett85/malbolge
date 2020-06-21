/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#ifndef VIRTUAL_CPU_MALBOLGE_HPP
#define VIRTUAL_CPU_MALBOLGE_HPP

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
    /** Assign the
     *
     * @param vmem Virtual memory containing the initialised memory space (and
     * program data)
     */
    explicit virtual_cpu(virtual_memory vmem) :
        vmem_(std::move(vmem))
    {}

    virtual_cpu(virtual_cpu&&) = default;
    virtual_cpu& operator=(virtual_cpu&&) = default;

    virtual_cpu(const virtual_cpu&) = delete;
    virtual_cpu& operator=(const virtual_cpu&) = delete;

    /** Execute the program.
     *
     * @throw std::logic_error Thrown if there is logic error within the
     * program.
     */
    void run();

private:
    virtual_memory vmem_;
};
}

#endif // VIRTUAL_CPU_MALBOLGE_HPP
