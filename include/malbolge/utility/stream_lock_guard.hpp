/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <iostream>
#include <functional>

/** A std::lock_guard clone that only acquires the lock if @a stream is
 * not in the standard library.
 *
 * <TT>std::cout</TT>, <TT>std::cerr</TT>, <TT>std::clog</TT>, and
 * <TT>std::cin</TT> are threadsafe - but <TT>std::iostream</TT> is not.  This
 * lock guard will not acquire the lock if the stream is a standard one.
 *
 * This class can be moved, but not copied.
 * @tparam Mutex Mutex type
 */
template <typename Mutex>
class stream_lock_guard
{
public:
    /** Constructor.
     *
     * @a mtx is acquired if @a stream is not a standard library one.
     * @tparam CharT Stream character type
     * @param mtx Mutex instance
     * @param stream Stream instance
     */
    template <typename CharT>
    stream_lock_guard(Mutex& mtx, std::basic_ios<CharT>& stream) :
        mtx_{mtx},
        locked_{false}
    {
        if (!is_standard_stream(stream)) {
            locked_ = true;
            mtx_.get().lock();
        }
    }

    /** Destructor.
     *
     * Unlocks the mutex if it was acquired in the constructor.
     */
    ~stream_lock_guard()
    {
        if (locked_) {
            mtx_.get().unlock();
        }
    }

    stream_lock_guard(stream_lock_guard&&) = default;
    stream_lock_guard(const stream_lock_guard&) = delete;

    stream_lock_guard& operator=(stream_lock_guard&&) = default;
    stream_lock_guard& operator=(const stream_lock_guard&) = delete;

private:
    template <typename CharT>
    static bool is_standard_stream(std::basic_ios<CharT>& stream)
    {
        return stream.rdbuf() == std::cin.rdbuf() ||
               stream.rdbuf() == std::cout.rdbuf() ||
               stream.rdbuf() == std::cerr.rdbuf() ||
               stream.rdbuf() == std::clog.rdbuf();
    }

    std::reference_wrapper<Mutex> mtx_;
    bool locked_;
};
