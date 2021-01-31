/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <atomic>
#include <mutex>
#include <functional>
#include <memory>

#include <boost/container/flat_map.hpp>

// It would have been nice to use boost::signals2 for this, but only the latest
// version (1.75) compiles in C++20 mode.  So I've gone for creating a simple
// one myself instead of mandating a cutting-edge Boost version
namespace malbolge
{
namespace utility
{
/** Signal type.
 *
 * When called, calls each of the connected slot functions and passes copies of
 * @a Args instances to each one.
 *
 * This class is threadsafe.  This class can be copied and moved.
 * @tparam Args Argument types
 */
template <typename... Args>
class signal
{
    struct impl_t;

public:
    /** Slot function signature.
     */
    using slot_type = std::function<void (Args...)>;

    /** Represents a signal/slot connection.
     *
     *  This class can be copied and moved.
     */
    class connection
    {
        friend class signal;

    public:
        /** Constructor.
         *
         *  Does nothing, and calling disconnect() on an instance constructed
         *  using this is a no-op.  This is only provided so that a connection
         *  instance can more easily be used a member.
         */
        connection() noexcept = default;

        /** Disconnects the slot from the signal.
         *
         * @note This has no effect if the owner has been destroyed, or was
         * never set
         */
        void disconnect()
        {
            auto owner = owner_.lock();
            if (owner) {
                auto lk = std::lock_guard{owner->mtx};
                owner->fns.erase(id_);
            }
        }

    private:
        connection(std::weak_ptr<typename signal::impl_t> owner,
                   std::size_t id) noexcept :
            owner_{std::move(owner)},
            id_{id}
        {}

        std::weak_ptr<typename signal::impl_t> owner_;
        std::size_t id_;
    };

    /** Default constructor.
     */
    explicit signal() :
        impl_{std::make_shared<impl_t>()}
    {}

    /** Copy Constructor.
     *
     * @param other Instance to copy from
     */
    signal(const signal& other) :
        signal()
    {
        auto other_lk = std::lock_guard{other.impl_->mtx};
        auto lk = std::lock_guard{impl_->mtx};

        impl_->new_id = other.impl_->new_id;
        impl_->fns = other.impl_->fns;
    }

    /** Move constructor.
     *
     * @param other Instance to move from
     */
    signal(signal&& other) noexcept :
        impl_{std::move(other.impl_)}
    {}

    signal& operator=(const signal& other) = default;
    signal& operator=(signal&& other) noexcept = default;

    /** Connect @a slot to this signal.
     *
     * @param slot Slot to be called when the signal is fired
     * @return Connection instance
     */
    connection connect(slot_type slot)
    {
        auto lk = std::lock_guard{impl_->mtx};

        auto& id = impl_->new_id;
        auto slot_ptr = std::make_shared<slot_type>(std::move(slot));
        impl_->fns.emplace(id, std::move(slot_ptr));

        return {std::weak_ptr<impl_t>{impl_}, id++};
    }

    /** Call each of the connected slots with @a args.
     *
     * @param args Arguments to pass to each slot
     */
    void operator()(Args... args)
    {
        // Make a shallow copy of the slot map so that we don't have the lock
        // held whilst calling the slots
        auto fns = fn_map{};
        {
            auto lk = std::lock_guard{impl_->mtx};
            fns = impl_->fns;
        }

        for (auto [id, fn] : fns) {
            (*fn)(args...);
        }
    }

private:
    using fn_map = boost::container::flat_map<size_t, std::shared_ptr<slot_type>>;

    struct impl_t
    {
        friend class connection;

        impl_t() :
            new_id{0}
        {
            // Avoid re-allocation for common scenarios
            fns.reserve(5);
        }

        std::mutex mtx;
        std::size_t new_id;
        fn_map fns;
    };

    std::shared_ptr<impl_t> impl_;
};
}
}
