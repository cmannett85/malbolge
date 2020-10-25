/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "math/ternary.hpp"

#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace malbolge
{
/** Namespace for debugging types.
 */
namespace debugger
{
/** Namespace for types representing registes and their properties.
 */
namespace vcpu_register
{
/** vCPU register identifiers.
 */
enum id {
    A,              ///< Accumulator
    C,              ///< Code pointer
    D,              ///< Data pointer
    NUM_REGISTERS   ///< Number of registers
};

/** Textual streaming operator for vcpu_register::id.
 *
 * @param stream Output stream
 * @param register_id Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, const id& register_id);

/** Data associated with a register i.e. its value and optional address.
 *
 * If this represents the:
 * - A register then only the value field is populated, which contains the value
 * of the register
 * - C or D register then the address field holds the address in the register,
 * and the value field holds the value in the memory location pointed at by the
 * address
 */
struct data
{
    /** Constructor.
     *
     * @note This constructor is only useful for the C and D register
     * @param a Address held in register
     * @param v Value at @a a
     */
    data(math::ternary a, math::ternary v) :
        address{a},
        value{v}
    {}

    /** Constructor.
     *
     * @note This constructor is only useful for the A register
     * @param v Value in the register
     */
    explicit data(math::ternary v = 0) :
        value{v}
    {}

    /** Comparison operator.
     *
     * The compiler generated version fails when built through Emscripten, hence
     * the custom implementation.  It will be replaced with the default version
     * once Emscripten (or rather its built-in version of Clang) is upgraded.
     * @param other Instance to compare against
     * @return Ordering
     */
    auto operator<=>(const data& other) const
    {
        const auto this_addr  = address ? *address :
                                          math::ternary{};
        const auto other_addr = other.address ? *(other.address) :
                                                math::ternary{};

        if (const auto cmp = this_addr <=> other_addr; cmp != 0) {
            return cmp;
        }
        return value <=> other.value;
    }

    bool operator==(const data& other) const = default;

    /** Register address, only valid when the register is C or D.
     */
    std::optional<math::ternary> address;

    /** Register value.
     */
    math::ternary value;
};

/** Textual streaming operator for vcpu_register::data.
 *
 * @param stream Output stream
 * @param d Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, const data& d);
}

/** vCPU control interface.
 *
 * A vCPU will populate an instance of this struct and return to client_control,
 * allowing client_control to control the vCPU's state without knowledge of the
 * vCPU itself.
 */
struct vcpu_control
{
    /** Pause the program.
     *
     * This is a no-op if the program is already paused.
     * @note This is asynchronous as the vCPU will be stopped on the next cycle
     */
    std::function<void ()> pause;

    /** Execution one more instruction.
     *
     * If the program is running when this is called, it will execute one more
     * instruction from its current instruction and then pause.
     */
    std::function<void ()> step;

    /** Resume program execution from a paused state.
     *
     * No-op if the program is not in a paused state.
     */
    std::function<void ()> resume;

    /** Returns the value at a given vmem address.
     *
     * @note Result is undefined if program is not paused
     * @param address vmem address
     * @return Value at @a address
     */
    std::function<math::ternary (math::ternary address)> address_value;

    /** Returns the address and/or value of a given register.
     *
     * @note Result is undefined if program is not paused
     * @param reg Register to query
     * @return For C and D registers returns the address held in the register
     * and the value it points at, for the A register it only returns the value
     */
    std::function<vcpu_register::data (vcpu_register::id reg)> register_value;
};

/** Debugger interface to the vCPU.
 *
 * Allows the attaching of breakpoints and execution control of a program
 * running on a vCPU.
 *
 * This class can be moved, but not copied.
 */
class client_control
{
public:
    /** Execution state of the debugged program.
     *
     * @note This is independent of the virtual_cpu::execution_state
     */
    enum class execution_state {
        NOT_RUNNING,    ///< Program stopped, or not started yet
        RUNNING,        ///< Program running
        PAUSED,         ///< Program paused
        NUM_STATES      ///< Number of execution states
    };

    /** Represents a breakpoint.
     *
     * @note Calling into client_control methods from a breakpoint callback will
     * cause a deadlock
     */
    class breakpoint
    {
    public:
        /** Callback function signature.
         *
         * This can be used to selectively pause the program.  When the
         * breakpoint is hit, this Callable is called with the address of the
         * breakpoint and register that is pointing to it (can only be C or D).
         * @param address Breakpoint vmem address
         * @param reg Register that triggered the breakpoint, C or D
         * @return True to stop execution, false to continue
         */
        using callback_type = std::function<bool (math::ternary address,
                                                  vcpu_register::id reg)>;

        /** A default callback that always pauses the program when hit (assuming
         * the ignore count has been reached).
         */
        static callback_type default_callback;

        /** Constructor.
         *
         * @param address vmem address to attach to
         * @param callback Callback to fire when triggered
         * @param ignore_count Ignore count, number times to hit before being
         * triggered
         */
        breakpoint(math::ternary address,
                   callback_type callback = default_callback,
                   std::size_t ignore_count = 0) :
            address_{address},
            cb_{std::move(callback)},
            ic_{ignore_count},
            count_{0}
        {}

        /** Address the breakpoint is attached to.
         *
         * @return vmem address
         */
        math::ternary address() const
        {
            return address_;
        }

        /** Call the callback associated with the breakpoint.
         *
         * @note If the call count is less than the ignore count then this
         * function returns false immediately
         * @param reg Register that triggered the breakpoint, C or D
         * @return True to stop execution, false to continue
         */
        bool operator()(vcpu_register::id reg) const
        {
            if (++count_ > ic_) {
                return cb_(address_, reg);
            }

            return false;
        }

        /** Number of times the breakpoint is hit before calling the trigger
         * callback.
         *
         * @return Ignore count
         */
        std::size_t ignore_count() const
        {
            return ic_;
        }

    private:
        math::ternary address_;
        callback_type cb_;
        std::size_t ic_;
        mutable std::size_t count_;
    };

    /** Constructor.
     *
     * The only requirement that a vCPU has is to have a confiqure_debugger
     * method which accepts a running state callback and a step data callback,
     * and returns a populated vcpu_control instance.  See
     * virtual_cpu::configure_debugger(running_callback_type, step_data_callback_type)
     * for an example.
     * @tparam vCPU vCPU type
     * @param vcpu vCPU instance to attach to
     * @exception basic_exception Thrown if @a vpu is already running, or
     * already has a debugger attached, or the vcpu did not populate all of the
     * vcpu_control fields
     */
    template <typename vCPU>
    explicit client_control(vCPU& vcpu) :
        state_{execution_state::NOT_RUNNING}
    {
        control_ = vcpu.configure_debugger(
            [this](execution_state state) {
                    auto lock = std::lock_guard{mtx_};
                    state_ = state;
            },
            [this](math::ternary address, vcpu_register::id reg) {
                    return check_vcpu_step(address, reg);
            }
        );

        validate_vcpu_control();
    }

    /** Returns the debugged program execution state.
     *
     * @return Execution state
     */
    execution_state state() const
    {
        return state_;
    }

    /** Pause the program.
     *
     * This is a no-op if the program is already paused.
     * @exception basic_exception Thrown if the program has stopped or not been
     * started
     */
    void pause();

    /** Execute one more instruction.
     *
     * If the program is running when this is called, it will execute one more
     * instruction from its current instruction and then pause.
     * @exception basic_exception Thrown if not in a paused state
     */
    void step();

    /** Resume program execution from a paused state.
     *
     * This is a no-op if the program is already running.
     * @note This will not start a stopped program.
     * @exception basic_exception Thrown if the program has stopped or not been
     * started
     */
    void resume();

    /** Returns the value at a given vmem address.
     *
     * @param address vmem address
     * @return Value at @a address
     * @exception basic_exception Thrown if program is running
     */
    math::ternary address_value(math::ternary address);

    /** Returns the address and/or value of a given register.
     *
     * @param reg Register to query
     * @return For C and D registers returns the address held in the register
     * and the value it points at, for the A register it only returns the value
     * @exception basic_exception Thrown if program is running
     */
    vcpu_register::data register_value(vcpu_register::id reg);

    /** Adds a breakpoint.
     *
     * @note This will silently overwrite an existing breakpoint if one is
     * already present at the same address
     * @param bp Breakpoint to add
     */
    void add_breakpoint(breakpoint bp);

    /** Removes a breakpoint at the given address.
     *
     * @param address vmem address to remove a breakpoint from
     * @return True if a breakpoint was removed, false if there was no
     * breakpoint at @a address
     */
    bool remove_breakpoint(math::ternary address);

    client_control(const client_control&) = delete;
    client_control& operator=(const client_control&) = delete;

private:
    bool check_vcpu_step(math::ternary address, vcpu_register::id reg);
    void validate_vcpu_control();

    std::unordered_map<math::ternary, breakpoint> breakpoints_;
    execution_state state_;
    vcpu_control control_;
    std::mutex mtx_;
};

/** Textual streaming operator for client_control::execution_state.
 *
 * @param stream Output stream
 * @param state Instance to stream
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream,
                         const client_control::execution_state& state);
}
}
