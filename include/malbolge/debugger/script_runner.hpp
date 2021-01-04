/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/utility/string_constant.hpp"
#include "malbolge/utility/tuple_iterator.hpp"
#include "malbolge/utility/visit.hpp"

#include <variant>
#include <iostream>

namespace malbolge
{
namespace debugger
{
/** Namespace for debugger scripting functions and types.
 */
namespace script
{
/** Namespace for debugger script types.
 */
namespace type
{
    using uint = std::uint32_t;             ///< 32-bit unsigned int
    using ternary = math::ternary;          ///< 10-trit ternary
    using reg = virtual_cpu::vcpu_register; ///< vCPU register
    using string = std::string;             ///< String

    /** A tuple type of all valid types.
     */
    using all = std::tuple<
            uint,
            ternary,
            reg,
            string
        >;
}

/** Represents a generic script function argument.
 *
 * @note If T is a string, then DefaultValue cannot be one because a string's
 * value cannot be defined at compile time.  In that case use a string_constant
 *
 * @tparam T Argument type, must be one of the types in script::type
 * @tparam Name Argument name, expected to be a string_constant or equivalent
 * @tparam DefaultValue The argument's value if one is not specified in the
 * constructor
 */
template <typename T, typename Name, typename DefaultValue = T>
class function_argument
{
public:
    using value_type = T;   ///< Argument's value type
    using name_type = Name; ///< Argument's name type

    /** Returns the name.
     *
     * @return Argument name
     */
    static constexpr std::string_view name()
    {
        return name_type::value();
    }

    /** Constructor
     *
     * @param v Value to assign
     */
    constexpr function_argument(value_type v = DefaultValue{}) :
        value(std::move(v))
    {}

    /** Comparison operator.
     *
     * @param other Instance to compare against
     * @return Ordering
     */
    auto operator<=>(const function_argument& other) const = default;

    /** Function value.
     */
    value_type value;

private:
    template <typename CheckT>
    using type_checker = std::is_same<CheckT, T>;

    static constexpr auto is_valid_type = boost::mp11::mp_any_of<
            type::all,
            type_checker
        >::value;
    static_assert(is_valid_type, "Function argument type must be one of "
                                 "debugger::script::type");
};

/** Textual streaming operator for function_argument.
 *
 * @tparam T Argument type, must be one of the types in script::type
 * @tparam Name Argument name, expected to be a string_constant or equivalent
 * @tparam DefaultValue The argument's value if one is not specified in the
 * constructor
 * @param stream Output stream
 * @param arg Instance to stream
 * @return @a stream
 */
template <typename T, typename Name, typename DefaultValue>
std::ostream& operator<<(std::ostream& stream,
                         const function_argument<T, Name, DefaultValue>& arg)
{
    return stream << arg.name() << "=" << arg.value;
}

/** Represents a script function.
 *
 * Argument names must be unique, it is a compilation failure otherwise.
 * @tparam Name Argument name, expected to be a string_constant or equivalent
 * @tparam Args List of function_argument types, or equivalent
 */
template <typename Name, typename... Args>
class function
{
    template <typename CheckedArg>
    struct name_check
    {
        template <typename Arg>
        struct checker : std::is_same<CheckedArg, typename Arg::name_type>
        {};
    };

public:
    using name_type = Name;                 ///< Argument's name type
    using args_type = std::tuple<Args...>;  ///< Tuple of function arguments

    /** Constructor.
     *
     * Each constructor argument can either be:
     * - A function_argument specialisation, in which case it can appear
     *   anywhere in the sequence as the name will be used to assign it
     * - A type implicitly convertible to a function_argument, in which case
     *   is has to appear in the order the function_arguments are specified
     * Any remaining unassigned arguments are assigned their default value.  It
     * is a compliation failure if there are more @a cargs than
     * function_arguments.
     *
     * It is a compilation failure if an unknown function_argument type is
     * provided.
     * @tparam ConArgs Constructor argument types
     * @param cargs Constructor function arguments
     */
    template <typename... ConArgs>
    constexpr function(ConArgs... cargs)
    {
        static_assert(sizeof...(ConArgs) <= sizeof...(Args),
                      "Number of ConArgs must not be greater than Args");

        utility::tuple_iterator([&](auto i, auto&& v) {
            using Arg = std::decay_t<decltype(v)>;

            if constexpr (traits::is_specialisation_v<Arg, function_argument>) {
                const_cast<Arg&>(arg<typename Arg::name_type>()) = std::move(v);
            } else {
                std::get<i>(args).value = std::move(v);
            }
        }, cargs...);
    }

    /** Returns a function_argument with the given name.
     *
     * It is a compilation failure if @a Name is unknown.
     * @tparam Name Argument name, expected to be a string_constant or
     * equivalent
     * @return Argument instance reference
     */
    template <typename ArgName>
    constexpr const auto& arg() const
    {
        using index = boost::mp11::mp_find_if<
                args_type,
                name_check<ArgName>::template checker
            >;
        static_assert(index::value != std::tuple_size_v<args_type>,
                      "Unknown argument name");

        return std::get<index::value>(args);
    }

    /** Returns the value of the given argument.
     *
     * It is a compilation failure if @a Name is unknown.
     * @tparam Name Argument name, expected to be a string_constant or
     * equivalent
     * @return Argument instance value
     */
    template <typename ArgName>
    constexpr const auto& value() const
    {
        return arg<ArgName>().value;
    }

    /** Returns the name.
     *
     * @return Argument name
     */
    static constexpr std::string_view name()
    {
        return name_type::value();
    }

    /** Comparison operator.
     *
     * @param other Instance to compare against
     * @return Ordering
     */
    auto operator<=>(const function& other) const = default;

    /** Function arguments.
     */
    args_type args;

private:
    template <typename Arg>
    struct unique_name
    {
        static constexpr auto value = std::is_same_v<
                boost::mp11::mp_count_if<args_type,
                                         name_check<typename Arg::name_type>::template checker>,
                boost::mp11::mp_size_t<1>
            >;
    };

    static constexpr auto has_unique_names = boost::mp11::mp_all_of<
            args_type,
            unique_name
        >::value;
    static_assert(has_unique_names, "Function must have unique argument names");
};

/** Textual streaming operator for function.
 *
 * @tparam Name Argument name, expected to be a string_constant or equivalent
 * @tparam Args List of function_argument types, or equivalent
 * @param stream Output stream
 * @param fn Instance to stream
 * @return @a stream
 */
template <typename Name, typename... Args>
std::ostream& operator<<(std::ostream& stream,
                         const function<Name, Args...>& fn)
{
    stream << fn.name() << "(";
    utility::tuple_iterator([&](auto i, auto&& v) {
        stream << v;
        if constexpr (i != (sizeof...(Args)-1)) {
            stream << ", ";
        }
    }, fn.args);

    return stream << ")";
}

/** Namespace holding the current script function types.
 */
namespace functions
{
/** Script function type for adding a breakpoint.
 *
 * Allows the adding of a breakpoint at any point during script execution.  The
 * breakpoint address is specified using the address argument (required); the
 * ignore count can be specified, otherwise it defaults to zero.
 */
using add_breakpoint = function<
    MAL_STR(add_breakpoint),
    function_argument<type::ternary, MAL_STR(address)>,
    function_argument<type::uint, MAL_STR(ignore_count),
                      traits::integral_constant<0>>
>;

/** Script function type for removing a breakpoint.
 *
 * Allows the removal of a breakpoint at any point during script execution.  The
 * breakpoint address is specified using the address argument (required).
 */
using remove_breakpoint = function<
    MAL_STR(remove_breakpoint),
    function_argument<type::ternary, MAL_STR(address)>
>;

/** Script function type for running the program.
 *
 * Once this is called in the sequence, subsequent functions are called once a
 * breakpoint hits.  The optional max_runtime argument puts an upperbound on
 * how long the program can run (in milliseconds) without  a breakpoint being
 * hit.  The timeout will trigger virtual_cpu::stop() being called.
 */
using run = function<
    MAL_STR(run),
    function_argument<type::uint, MAL_STR(max_runtime_ms),
                      traits::integral_constant<0>>
>;

/** Script function type querying the value in vmem at a particular address.
 */
using address_value = function<
    MAL_STR(address_value),
    function_argument<type::ternary, MAL_STR(address)>
>;

/** Script function type querying the value in at the given register.
 *
 * If the register is C or D, then the register contains an address - this is
 * printed along with the value at the address in vmem.
 */
using register_value = function<
    MAL_STR(register_value),
    function_argument<type::reg, MAL_STR(reg)>
>;

/** Script function type for stepping the program a single instruction.
 */
using step = function<
    MAL_STR(step)
>;

/** Script function type for resuming a paused program.
 *
 * Once this is called in the sequence, subsequent functions are called once a
 * breakpoint hits.
 */
using resume = function<
    MAL_STR(resume)
>;

/** Script function type for passing a string for the program to accept as
 * input.
 *
 * The argument is added to an input queue, so this may be called multiple
 * times to 'pre-load' input.
 */
using on_input = function<
    MAL_STR(on_input),
    function_argument<type::string, MAL_STR(data)>
>;

/** A variant of all the known functions.
 */
using function_variant = std::variant<
    add_breakpoint,
    remove_breakpoint,
    run,
    address_value,
    register_value,
    step,
    resume,
    on_input
>;

/** A sequence of functions, which defines a debugger script.
 *
 * There are restrictions on the ordering of certain functions, which are
 * validated when the sequence is ran.  Those restrictions are:
 *  - There is one, and only one, run function
 *  - A step or resume function does not appear before a run
 *  - If there are any add_breakpoint functions, at least one must appear before
 *    a run
 */
using sequence = std::vector<function_variant>;
}

/** Textual streaming operator.
 *
 * @param stream Output stream
 * @param fn Function variant
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream,
                         const functions::function_variant& fn);

/** Textual streaming operator.
 *
 * @param stream Output stream
 * @param seq Sequence
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, const functions::sequence& seq);

/** A wrapper around a virtual_cpu that allows a function sequence to be
 *  executed on it.
 *
 * This class can be moved but not copied.
 */
class script_runner
{
public:
    /** Signal type carrying program output data.
     */
    using output_signal_type = utility::signal<char>;

    /** Signal type carrying the value resulting from an address_value function.
     *
     * @tparam functions::address_value Function
     * @tparam math::ternary Value
     */
    using address_value_signal_type = utility::signal<functions::address_value,
                                                      math::ternary>;

    /** Signal type carrying the value resulting from a register_value function.
     *
     * @tparam functions::address_value Function
     * @tparam std::optional<math::ternary> If the register is C or D then it
     * contains an address
     * @tparam math::ternary The value of the register if the address is empty,
     * otherwise the value at the address
     */
    using register_value_signal_type = utility::signal<functions::register_value,
                                                       std::optional<math::ternary>,
                                                       math::ternary>;

    /** Constructor.
     */
    explicit script_runner() = default;

    script_runner(script_runner&& other) = default;
    script_runner& operator=(script_runner&& other) = default;
    script_runner(const script_runner&) = delete;
    script_runner& operator=(const script_runner&) = delete;

    /** Runs @a fn_seq on an internal vCPU.
     *
     * This is a blocking call.  The internal vCPU's lifetime is the same as
     * this function's, this means that the script_runner instance can be
     * reused.
     * @param vmem Initialised virtual memory
     * @param fn_seq Function sequence
     * @exception base_exception Thrown if @a fn_seq is malformed, or the
     * program execution fails
     */
    void run(virtual_memory vmem, const functions::sequence& fn_seq);

    /** Register @a slot to be called when the output signal fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    output_signal_type::connection
    register_for_output_signal(output_signal_type::slot_type slot)
    {
        return output_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the address value result signal
     *  fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    address_value_signal_type::connection
    register_for_address_value_signal(address_value_signal_type::slot_type slot)
    {
        return address_sig_.connect(std::move(slot));
    }

    /** Register @a slot to be called when the register value result signal
     *  fires.
     *
     * You can disconnect from the signal using the returned connection
     * instance.
     * @note @a slot is called from the vCPU's local event loop thread, so you
     * may need to post into the event loop you intend on processing it with
     * @param slot Callable instance called when the signal fires
     * @return Connection data
     */
    register_value_signal_type::connection
    register_for_register_value_signal(register_value_signal_type::slot_type slot)
    {
        return reg_sig_.connect(std::move(slot));
    }

private:
    output_signal_type output_sig_;
    address_value_signal_type address_sig_;
    register_value_signal_type reg_sig_;
};
}
}
}
