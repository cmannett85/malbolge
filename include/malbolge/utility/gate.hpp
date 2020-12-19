/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

namespace malbolge
{
namespace utility
{
/** Allows one thread to control flow of execution in another.
 *
 * The controlling thread uses open(std::size_t) and close() to free/block the
 * controlled thread (a gate instance is shared between them).  The controlled
 * thread uses operator()() to notify the controller when execution passes
 * through the gate.
 *
 * Behaviour is undefined is the number of controlled threads is greater than
 * one.
 *
 * This class can be moved, but not copied.
 */
class gate
{
public:
    /** A callback type that can be used to notify when a gate is closed and
     * opened.
     *
     * This is called just before the controlled thread blocks, and again when
     * unblocked.  It is not called if the gate is open.
     * @param closed True when the gate is closed and will block, false when
     * re-opened
     */
    using notifier_type = std::function<void (bool closed)>;

    /** Represents an always open gate, until it is explicitly closed.
     */
    static constexpr auto always_allow = std::int64_t{-1};

    /** Constructor.
     *
     * The gate starts open with an unlimited number of operator()() calls
     * i.e. open().
     */
    explicit gate() :
        sync_{std::make_unique<sync>()},
        allow_{always_allow}
    {}

    /** Destructor.
     *
     * Always opens the gate before returning.
     */
    ~gate()
    {
        // If this is the destruction of a moved-from instance then sync_ will
        // be null
        if (sync_) {
            open();
        }
    }

    /** Called by the controlled thread to allow flow of execution or block it.
     *
     * @note The gate's mutex is locked when the notifier is called, so do not
     * call any gate methods from the notifier
     * @param notifier If set, and the gate will be closed, it is called.  It
     * is called again once unlocked
     */
    void operator()(const notifier_type& notifier = {})
    {
        auto lock = std::unique_lock{sync_->mtx_};
        if (allow_ == always_allow) {
            return;
        }

        if (allow_ > 0) {
             --allow_;
        }

        const auto notify = !allow_ && notifier;
        if (notify) {
            notifier(true);
        }

        // Block if allowed count reaches zero
        sync_->cv_.wait(lock, [this]() { return allow_; });

        if (notify) {
            notifier(false);
        }
    }

    /** Opens the gate.
     *
     * @param close_after Number of operator()() calls allowed before the gate
     * closes, or always_allow to stay open until close() is called
     */
    void open(std::int64_t close_after = always_allow)
    {
        {
            auto lock = std::lock_guard{sync_->mtx_};
            allow_ = close_after;
        }
        sync_->cv_.notify_one();
    }

    /** Closes the gate.
     */
    void close()
    {
        {
            auto lock = std::lock_guard{sync_->mtx_};
            allow_ = 0;
        }
        sync_->cv_.notify_one();
    }

    gate(const gate&) = delete;
    gate& operator=(const gate&) = delete;

    gate(gate&&) = default;
    gate& operator=(gate&&) = default;

private:
    struct sync
    {
        std::mutex mtx_;
        std::condition_variable cv_;
    };

    std::unique_ptr<sync> sync_;
    std::int64_t allow_;
};
}
}
