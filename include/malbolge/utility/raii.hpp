/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <functional>

namespace malbolge
{
/** Namespace for not-Malbolge-specific utility functions and types.
 */
namespace utility
{
/** A wrapper over a Callable instance that is executed upon destruction.
 *
 *  This class can be copied and moved.
 */
class raii
{
public:
    /** Type of the held Callable.
     */
    using callable_type = std::function<void ()>;

    /** Constructor.
     *
     * @param f Callable instance to execute upon destruction
     */
    explicit raii(callable_type f) noexcept :
        f_{std::move(f)}
    {}

    /** Destructor.
     *
     * If the Callable instance pass into the constructor is valid, it will be
     * executed.
     */
    ~raii()
    {
        if (f_) {
            f_();
        }
    }

private:
    callable_type f_;
};
}
}
