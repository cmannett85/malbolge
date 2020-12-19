/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/client_control.hpp"
#include "malbolge/exception.hpp"

using namespace malbolge;

std::ostream& debugger::vcpu_register::operator<<(std::ostream& stream,
                                                  const id& register_id)
{
    static_assert(NUM_REGISTERS == 3, "Register IDs have changed, update this");

    switch(register_id) {
    case A:
        return stream << "A";
    case C:
        return stream << "C";
    case D:
        return stream << "D";
    default:
        return stream << "Unknown register ID: " << register_id;
    }
}

std::ostream& debugger::vcpu_register::operator<<(std::ostream& stream,
                                                  const data& d)
{
    if (d.address) {
        stream << "{" << *(d.address) << ", ";
    } else {
        stream << "{{}, ";
    }
    return stream << d.value << "}";
}

debugger::client_control::breakpoint::callback_type
debugger::client_control::breakpoint::default_callback =
        [](math::ternary, debugger::vcpu_register::id) {
    return true;
};

void debugger::client_control::pause(vcpu_control::callback_type cb)
{
    {
        auto lock = std::lock_guard{mtx_};
        if (state_ == execution_state::NOT_RUNNING) {
            throw basic_exception{"Program not running"};
        }
    }

    // Wrap the callback so that we can set the debugger state
    cb = [this, user_cb = std::move(cb)]() {
        if (user_cb) {
            user_cb();
        }

        // No need to lock mtx_ as it is already locked when called in
        // check_vcpu_step(..)
        state_ = execution_state::PAUSED;
        return;
    };

    control_.pause(std::move(cb));
}

void debugger::client_control::step(vcpu_control::callback_type cb)
{
    {
        auto lock = std::lock_guard{mtx_};
        if (state_ != execution_state::PAUSED) {
            throw basic_exception{"Program not paused"};
        }
    }

    // Wrap the callback so that we can set the debugger state
    cb = [this, user_cb = std::move(cb)]() {
        if (user_cb) {
            user_cb();
        }

        // No need to lock mtx_ as it is already locked when called in
        // check_vcpu_step(..)
        state_ = execution_state::PAUSED;
        return;
    };

    control_.step(std::move(cb));
}

void debugger::client_control::resume(vcpu_control::callback_type cb)
{
    {
        auto lock = std::lock_guard{mtx_};
        if (state_ == execution_state::NOT_RUNNING) {
            throw basic_exception{"Program not running"};
        }
    }

    // Wrap the callback so that we can set the debugger state
    cb = [this, user_cb = std::move(cb)]() {
        if (user_cb) {
            user_cb();
        }

        // No need to lock mtx_ as it is already locked when called in
        // check_vcpu_step(..)
        state_ = execution_state::RUNNING;
        return;
    };

    control_.resume(std::move(cb));
}

math::ternary
debugger::client_control::address_value(math::ternary address) const
{
    {
        auto lock = std::lock_guard{mtx_};
        if (state_ == execution_state::RUNNING) {
            throw basic_exception{"Program is running"};
        }
    }

    return control_.address_value(address);
}

debugger::vcpu_register::data
debugger::client_control::register_value(vcpu_register::id reg) const
{
    {
        auto lock = std::lock_guard{mtx_};
        if (state_ == execution_state::RUNNING) {
            throw basic_exception{"Program is running"};
        }
    }

    return control_.register_value(reg);
}

void debugger::client_control::add_breakpoint(breakpoint bp)
{
    auto lock = std::lock_guard{mtx_};

    // Wrap the breakpoint callback so that we can set the debugger state
    auto cb = [this, bp_cb = std::move(bp.cb_)](auto addr, auto reg) {
        const auto stop = bp_cb(addr, reg);

        // No need to lock mtx_ as it is already locked when called in
        // check_vcpu_step(..)
        state_ = stop ? execution_state::PAUSED : execution_state::RUNNING;
        return stop;
    };

    bp.cb_ = std::move(cb);

    breakpoints_.insert_or_assign(bp.address(), std::move(bp));
}

bool debugger::client_control::remove_breakpoint(math::ternary address)
{
    auto lock = std::lock_guard{mtx_};
    return breakpoints_.erase(address) > 0;
}

bool debugger::client_control::check_vcpu_step(math::ternary address,
                                               vcpu_register::id reg)
{
    auto lock = std::lock_guard{mtx_};

    auto it = breakpoints_.find(address);
    if (it == breakpoints_.end()) {
        return false;
    }

    return it->second(reg);
}

void debugger::client_control::validate_vcpu_control()
{
    // Check that the vcpu has populated all the fields
    if (!control_.pause         ||
        !control_.step          ||
        !control_.resume        ||
        !control_.address_value ||
        !control_.register_value) {
        throw basic_exception{"vCPU did not populate all of the vcpu_control fields"};
    }
}

std::ostream& debugger::operator<<(std::ostream& stream,
                                   const client_control::execution_state& state)
{
    static_assert(static_cast<int>(client_control::execution_state::NUM_STATES) == 3,
                  "Execution states have changed, update this");

    switch (state) {
    case client_control::execution_state::NOT_RUNNING:
        return stream << "NOT_RUNNING";
    case client_control::execution_state::RUNNING:
        return stream << "RUNNING";
    case client_control::execution_state::PAUSED:
        return stream << "PAUSED";
    default:
        return stream << "Unknown client_control execution state: "
                      << static_cast<int>(state);
    }
}
