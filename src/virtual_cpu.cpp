/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"
#include "malbolge/log.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>
#include <deque>
#include <unordered_map>

using namespace malbolge;

class virtual_cpu::impl_t : public std::enable_shared_from_this<impl_t>
{
public:
    class breakpoint
    {
    public:
        breakpoint(math::ternary a, std::size_t i = 0) :
            address_{a},
            ignore_count_{i},
            pre_{false}
        {}

        bool operator()()
        {
            if (pre_) {
                pre_ = false;
                return false;
            }

            if (ignore_count_ == 0) {
                pre_ = true;
                return true;
            }

            --ignore_count_;
            return false;
        }

    private:
        math::ternary address_;
        std::size_t ignore_count_;

        // The breakpoint is fired (and this variable set) on entry to an
        // address, so on the next run() call the breakpoint will be hit again -
        // this variable being set high will cause the breakpoint to be ignored.
        // This must be set low after, otherwise the breakpoint will never fire
        // again
        bool pre_;
    };

    class input
    {
    public:
        input(std::string p) :
            phrase_{std::move(p)},
            view_{phrase_.data()}
        {}

        char get()
        {
            if (view_.empty()) {
                return 0;
            }

            auto c = view_.front();
            view_.remove_prefix(1);
            return c;
        }

    private:
        std::string phrase_;
        std::string_view view_;
    };

    explicit impl_t(virtual_memory vm) :
        worker_guard_{ctx.get_executor()},
        vmem(std::move(vm)),
        c{vmem.begin()},
        d{vmem.begin()},
        p_counter{0},
        state_{virtual_cpu::execution_state::READY}
    {}

    virtual_cpu::execution_state state() const
    {
        return state_;
    }

    void set_state(virtual_cpu::execution_state new_state,
                   std::exception_ptr eptr = {})
    {
        if (state_ == new_state) {
            return;
        }

        state_ = new_state;
        state_sig(state_, eptr);
    }

    void stop()
    {
        worker_guard_.reset();
        ctx.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    void stopped_check()
    {
        if (state_ == virtual_cpu::execution_state::STOPPED) {
            throw execution_exception{"vCPU has been stopped", p_counter};
        }
    }

    bool bp_check(virtual_memory::iterator reg_it);

    void run(bool schedule_next = true);

    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker_guard_;
    std::thread thread;

    virtual_memory vmem;
    std::deque<input> input_queue_;
    std::unordered_map<math::ternary, breakpoint> bps;

    // vCPU Registers
    math::ternary a;
    virtual_memory::iterator c;
    virtual_memory::iterator d;
    std::size_t p_counter;

    state_signal_type state_sig;
    output_signal_type output_sig;
    breakpoint_hit_signal_type bp_hit_sig;

private:
    virtual_cpu::execution_state state_;
};

virtual_cpu::virtual_cpu(virtual_memory vmem) :
    impl_{std::make_shared<impl_t>(std::move(vmem))}
{
    impl_->thread = std::thread{[impl = impl_]() {
        auto eptr = std::exception_ptr{};
        try {
            impl->ctx.run();
        } catch (std::exception&) {
            eptr = std::current_exception();
        }
        impl->set_state(execution_state::STOPPED, eptr);
    }};
}

virtual_cpu::~virtual_cpu()
{
    if (!impl_) {
        return;
    }
    impl_->stop();
}

void virtual_cpu::run()
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_]() {
        impl->stopped_check();
        if (impl->state() == execution_state::RUNNING ||
            impl->state() == execution_state::WAITING_FOR_INPUT) {
            return;
        }

        impl->set_state(execution_state::RUNNING);
        impl->run();
    });
}

void virtual_cpu::pause()
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_]() {
        impl->stopped_check();
        if (impl->state() == execution_state::PAUSED ||
            impl->state() == execution_state::WAITING_FOR_INPUT) {
            return;
        }

        impl->set_state(execution_state::PAUSED);
    });
}

void virtual_cpu::step()
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_]() {
        impl->stopped_check();
        if (impl->state() == execution_state::WAITING_FOR_INPUT) {
            return;
        }

        impl->set_state(execution_state::PAUSED);
        impl->run(false);
    });
}

void virtual_cpu::add_input(std::string data)
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_, data = std::move(data)]() {
        impl->input_queue_.emplace_back(std::move(data));
        if (impl->state() == execution_state::WAITING_FOR_INPUT) {
            impl->set_state(execution_state::RUNNING);
            impl->run();
        }
    });
}

void virtual_cpu::add_breakpoint(math::ternary address, std::size_t ignore_count)
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_, address, ignore_count]() {
        auto bp = impl_t::breakpoint{address, ignore_count};
        impl->bps.insert_or_assign(address, std::move(bp));
    });
}

void virtual_cpu::remove_breakpoint(math::ternary address)
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_, address]() {
        impl->bps.erase(address);
    });
}

void virtual_cpu::address_value(math::ternary address,
                                address_value_callback_type cb) const
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_, address, cb = std::move(cb)]() {
        const auto value = impl->vmem[address];
        cb(address, value);
    });
}

void virtual_cpu::register_value(vcpu_register reg,
                                 register_value_callback_type cb) const
{
    impl_check();
    boost::asio::post(impl_->ctx, [impl = impl_, reg, cb = std::move(cb)]() {
        switch (reg) {
        case vcpu_register::A:
            cb(reg, {}, impl->a);
            break;
        case vcpu_register::C:
        {
            const auto address = static_cast<math::ternary::underlying_type>(
                                    std::distance(impl->vmem.begin(), impl->c));
            cb(reg, address, impl->vmem[address]);
            break;
        }
        case vcpu_register::D:
        {
            const auto address = static_cast<math::ternary::underlying_type>(
                                    std::distance(impl->vmem.begin(), impl->d));
            cb(reg, address, impl->vmem[address]);
            break;
        }
        default:
            throw execution_exception{
                "Unhandled register query: " + std::to_string(static_cast<int>(reg)),
                impl->p_counter
            };
        }
    });
}

virtual_cpu::state_signal_type::connection
virtual_cpu::register_for_state_signal(state_signal_type::slot_type slot)
{
    impl_check();
    return impl_->state_sig.connect(std::move(slot));
}

virtual_cpu::output_signal_type::connection
virtual_cpu::register_for_output_signal(output_signal_type::slot_type slot)
{
    impl_check();
    return impl_->output_sig.connect(std::move(slot));
}

virtual_cpu::breakpoint_hit_signal_type::connection
virtual_cpu::register_for_breakpoint_hit_signal(breakpoint_hit_signal_type::slot_type slot)
{
    impl_check();
    return impl_->bp_hit_sig.connect(std::move(slot));
}

void virtual_cpu::impl_check() const
{
    if (!impl_) {
        throw execution_exception{"vCPU backend destroyed, use-after-move?", 0};
    }
}

bool virtual_cpu::impl_t::bp_check(virtual_memory::iterator reg_it)
{
    const auto address = static_cast<math::ternary::underlying_type>(
        std::distance(vmem.begin(), reg_it)
    );
    auto it = bps.find(address);
    if (it == bps.end()) {
        return false;
    } else if (it->second()) {
        set_state(virtual_cpu::execution_state::PAUSED);
        bp_hit_sig(address);
        return true;
    }
    return false;
}

void virtual_cpu::impl_t::run(bool schedule_next)
{
    // A pause() needs to break the run()-chain
    if (state_ == virtual_cpu::execution_state::PAUSED && schedule_next) {
        return;
    }

    if (bp_check(c)) {
        return;
    }

    // Pre-cipher the instruction
    auto instr = pre_cipher_instruction(*c, c - vmem.begin());
    if (!instr) {
        throw execution_exception{
            "Pre-cipher non-whitespace character must be graphical "
                "ASCII: " + std::to_string(static_cast<int>(*c)),
            p_counter
        };
    }

    log::print(log::VERBOSE_DEBUG,
               "Step: ", p_counter, ", pre-cipher instr: ",
               static_cast<int>(*instr));

    switch (*instr) {
    case cpu_instruction::set_data_ptr:
        d = vmem.begin() + static_cast<std::size_t>(*d);
        break;
    case cpu_instruction::set_code_ptr:
        c = vmem.begin() + static_cast<std::size_t>(*d);
        break;
    case cpu_instruction::rotate:
        a = d->rotate();
        break;
    case cpu_instruction::op:
        a = *d = a.op(*d);
        break;
    case cpu_instruction::read:
    {
        if (input_queue_.empty()) {
            set_state(virtual_cpu::execution_state::WAITING_FOR_INPUT);
            log::print(log::VERBOSE_DEBUG, "\tWaiting for input...");
            return;
        }

        auto c = input_queue_.front().get();
        if (!c) {
            a = math::ternary::max;
            input_queue_.pop_front();
        } else {
            a = c;
        }
        break;
    }
    case cpu_instruction::write:
        if (a != math::ternary::max) {
            output_sig(static_cast<char>(a));
        }
        break;
    case cpu_instruction::stop:
        set_state(virtual_cpu::execution_state::STOPPED);
        return;
    default:
        // Nop
        break;
    }

    // Post-cipher the instruction
    auto pc = post_cipher_instruction(*c);
    if (!pc) {
        throw execution_exception{
            "Post-cipher non-whitespace character must be graphical "
                "ASCII: " + std::to_string(static_cast<int>(*c)),
            p_counter
        };
    }
    *c = *pc;

    log::print(log::VERBOSE_DEBUG,
               "\tPost-op regs - a: ", a,
               ", c[", std::distance(vmem.begin(), c), "]: ", *c,
               ", d[", std::distance(vmem.begin(), d), "]: ", *d);

    ++c;
    ++d;
    ++p_counter;
    if (schedule_next) {
        // Schedule the next iteration
        boost::asio::post(ctx, [impl = shared_from_this()]() {
            impl->run();
        });
    }
}

std::ostream& malbolge::operator<<(std::ostream& stream,
                                   virtual_cpu::vcpu_register register_id)
{
    static_assert(static_cast<int>(virtual_cpu::vcpu_register::NUM_REGISTERS) == 3,
                  "Register IDs have changed, update this");

    switch(register_id) {
    case virtual_cpu::vcpu_register::A:
        return stream << "A";
    case virtual_cpu::vcpu_register::C:
        return stream << "C";
    case virtual_cpu::vcpu_register::D:
        return stream << "D";
    default:
        return stream << "Unknown register ID: " << register_id;
    }
}

std::ostream& malbolge::operator<<(std::ostream& stream,
                                   virtual_cpu::execution_state state)
{
    static_assert(static_cast<int>(virtual_cpu::execution_state::NUM_STATES) == 5,
                  "Number of execution states have change, update operator<<");

    switch (state) {
    case virtual_cpu::execution_state::READY:
        return stream << "READY";
    case virtual_cpu::execution_state::RUNNING:
        return stream << "RUNNING";
    case virtual_cpu::execution_state::PAUSED:
        return stream << "PAUSED";
    case virtual_cpu::execution_state::WAITING_FOR_INPUT:
        return stream << "WAITING_FOR_INPUT";
    case virtual_cpu::execution_state::STOPPED:
        return stream << "STOPPED";
    default:
        return stream << "Unknown";
    }
}
