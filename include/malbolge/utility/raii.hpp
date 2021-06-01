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
    explicit raii(callable_type f = {}) noexcept :
        f_{std::move(f)}
    {}

    raii(const raii& other) noexcept = default;

    raii(raii&& other) noexcept = default;

    raii& operator=(raii other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    /** Destructor.
     *
     * If the Callable instance pass into the constructor is valid, it will be
     * executed.
     */
    ~raii()
    {
        fire();
    }

    /** Replace the callable instance with another.
     *
     * A default constructed callable instance effectively deactivates the
     * raii instance.
     * @param f Callable instance to execute upon destruction
     */
    void reset(callable_type f = {}) noexcept
    {
        f_ = std::move(f);
    }

    /** Swap function.
     *
     * @param a First instance
     * @param b Second instance
     */
    friend void swap(raii& a, raii& b)
    {
        using std::swap;

        swap(a.f_, b.f_);
    }

private:
    void fire()
    {
        if (f_) {
            f_();
        }
    }

    callable_type f_;
};
}
}
