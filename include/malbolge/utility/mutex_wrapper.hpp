/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <functional>

namespace malbolge
{
namespace utility
{
/** Wraps a mutex type following the <TT>Mutex</TT> concept, in a generic type.
 *
 * This wrapper allows functions to receive generic mutex typees without being
 * templated themselves.
 *
 * It also allows for empty initialisation to mimic a dummy mutex (i.e. a mutex
 * type that doesn't actually do anything.
 */
class mutex_wrapper
{
public:
    /** Default constructor.
     *
     * All the functions are no-ops. try_lock() returns true.
     */
    mutex_wrapper() = default;

    /** Constructor.
     *
     * @tparam Mutex Mutex type
     * @param mtx Mutex reference
     */
    template <typename Mutex>
    mutex_wrapper(Mutex& mtx) :
        lock_{[&]() { mtx.lock(); }},
        unlock_{[&]() { mtx.unlock(); }},
        try_lock_{[&]() { return mtx.try_lock(); }}
    {}

    /** Returns true if this is wrapping a mutex.
     *
     * @return True if this is wrapping a mutex
     */
    bool is_valid() const
    {
        return !!lock_;
    }

    /** Implicit conversion operator to boolean.
     *
     * @return Returns true if this is wrapping a mutex
     */
    operator bool() const
    {
        return is_valid();
    }

    /** Returns true if this is not wrapping a mutex.
     *
     * @return True if this is not wrapping a mutex
     */
    bool operator!() const
    {
        return !is_valid();
    }

    /** Lock the mutex.
     */
    void lock()
    {
        if (is_valid()) {
            lock_();
        }
    }

    /** Unlock the mutex.
     */
    void unlock()
    {
        if (is_valid()) {
            unlock_();
        }
    }

    /** Try to lock to the mutex.
     *
     * @return True if the mutex was locked
     */
    bool try_lock()
    {
        if (is_valid()) {
            return try_lock_();
        }
        return true;
    }

private:
    [[no_unique_address]] std::function<void ()> lock_;
    [[no_unique_address]] std::function<void ()> unlock_;
    [[no_unique_address]] std::function<bool ()> try_lock_;
};
}
}
