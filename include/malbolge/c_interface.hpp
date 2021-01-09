/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

/** @file
 * Exposes a C89 interface to the important parts of the library.
 *
 * A C interface makes accessing functionality from WASM or creating bindings
 * to other languages much easier.  It is not threadsafe.
 */

extern "C"
{
/** Opaque handle for virtual memory.
 */
typedef void* malbolge_virtual_memory;

/** Opaque handle for a virtual CPU.
 */
typedef void* malbolge_virtual_cpu;

/** Enum for Malbolge return codes.
 *
 * Errors start at -0x1000, so standard platform error codes can be used as
 * well.
 */
enum malbolge_result
{
    MALBOLGE_ERR_TRUE                   =  0x0001, ///< True
    MALBOLGE_ERR_FALSE                  =  0x0000, ///< False

    MALBOLGE_ERR_SUCCESS                =  0x0000, ///< Success
    MALBOLGE_ERR_UNKNOWN                = -0x1000, ///< An unknown/generic error occurred
    MALBOLGE_ERR_INVALID_LOG_LEVEL      = -0x1001, ///< Invalid log level requested
    MALBOLGE_ERR_NULL_ARG               = -0x1002, ///< An input was unexpectedly NULL
    MALBOLGE_ERR_PARSE_FAIL             = -0x1003, ///< Program source parse failure
    MALBOLGE_ERR_EXECUTION_FAIL         = -0x1004, ///< Program execution failure
};

/** vCPU execution states.
 *
 * C equivalent to virtual_cpu::execution_state.
 */
enum malbolge_vcpu_execution_state
{
    MALBOLGE_VCPU_READY,                ///< Ready to run
    MALBOLGE_VCPU_RUNNING,              ///< Program running
    MALBOLGE_VCPU_PAUSED,               ///< Program paused
    MALBOLGE_VCPU_WAITING_FOR_INPUT,    ///< Similar to paused, except the program will
                                        ///< resume when input data provided
    MALBOLGE_VCPU_STOPPED,              ///< Program stopped, cannot be resumed or ran again
    MALBOLGE_VCPU_NUM_STATES            ///< Number of execution states
};

/** vCPU register identifiers for the debugger.
 *
 * C equivalent to virtual_cpu::vcpu_register.
 */
enum malbolge_vcpu_register
{
    MALBOLGE_VCPU_REGISTER_A,    ///< Accumulator
    MALBOLGE_VCPU_REGISTER_C,    ///< Code pointer
    MALBOLGE_VCPU_REGISTER_D,    ///< Data pointer
    MALBOLGE_VCPU_REGISTER_MAX,  ///< Number of registers
};

/** Program load normalised modes.
 *
 *  C equivalen to malbolge::load_normalised_mode.
 */
enum malbolge_load_normalised_mode
{
    MALBOLGE_LOAD_NORMALISED_AUTO,      ///< Automatically detect if normalised, uses
                                        ///< malbolge_is_likely_normalised_source(const char*, unsigned long)
    MALBOLGE_LOAD_NORMALISED_ON,        ///< Force load as normalised
    MALBOLGE_LOAD_NORMALISED_OFF,       ///< Force load as non-normalised
    MALBOLGE_LOAD_NORMALISED_NUM_MODES  ///< Number of normalisation modes
};

/** Function pointer signature for the vCPU execution state callback.
 *
 * This is equivalent to virtual_cpu::state_signal_type and
 * virtual_cpu::error_signal_type.
 * @param vcpu vCPU handle
 * @param state The new program state
 * @param error_code If an error occured, this will be less than zero
 */
typedef void (*malbolge_vcpu_state_callback)(malbolge_virtual_cpu vcpu,
                                             enum malbolge_vcpu_execution_state state,
                                             int error_code);

/** Function pointer signature for the vCPU output callback.
 *
 * This is equivalent to virtual_cpu::output_signal_type.
 * @param vcpu vCPU handle
 * @param c Character emitted from the program
 */
typedef void (*malbolge_vcpu_output_callback)(malbolge_virtual_cpu vcpu,
                                              char c);

/** Function pointer signature for the vCPU breakpoint hit callback.
 *
 * This is equivalent to virtual_cpu::breakpoint_hit_signal_type.
 * @param vcpu vCPU handle
 * @param address Address the breakpoint resides on
 */
typedef void (*malbolge_vcpu_breakpoint_hit_callback)(malbolge_virtual_cpu vcpu,
                                                      unsigned int address);

/** Function pointer signature for the vCPU address value callback.
 *
 * This is equivalent to virtual_cpu::address_value_callback_type.
 * @param vcpu vCPU handle
 * @param address The queried address
 * @param value Value extracted from the vmem address
 */
typedef void (*malbolge_vcpu_address_value_callback)(malbolge_virtual_cpu vcpu,
                                                     unsigned int address,
                                                     unsigned int value);

/** Function pointer signature for the vCPU register value callback.
 *
 * This is equivalent to virtual_cpu::register_value_callback_type.
 * @param vcpu vCPU handle
 * @param reg The queried vCPU register
 * @param address If @a reg is C or D then it contains an address, otherwise 0
 * @param value The value of the register if @a reg is A, otherwise the value
 * at @a address
 */
typedef void (*malbolge_vcpu_register_value_callback)(malbolge_virtual_cpu vcpu,
                                                      enum malbolge_vcpu_register reg,
                                                      unsigned int address,
                                                      unsigned int value);

/** Returns the current minimum logging level.
 *
 * The value from malbolge::log::log_level() is subtracted from
 * malbolge::log::ERROR and then returned.
 * @return Minimum logging level
 */
unsigned int malbolge_log_level();

/** Set the minimum logging level.
 *
 * @a level is subtracted from malbolge::log::ERROR, the result is passed to
 * malbolge::log::set_log_level(level).
 * @param level New minimum log level
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_INVALID_LOG_LEVEL if @a level is larger than the maximum
 */
int malbolge_set_log_level(unsigned int level);

/** Returns malbolge::version_string.
 *
 * The return is a string literal and so must not be user freed.
 * @return The full version string.
 */
const char *malbolge_version();

/** Returns true if the input source is likely to be normalised.
 *
 * See malbolge::is_likely_normalised_source(InputIt, InputIt) for further
 * explanation on this function.
 * @param buffer Program source
 * @param size Size in bytes of @a buffer
 * @return
 * - MALBOLGE_ERR_TRUE if normalised
 * - MALBOLGE_ERR_FALSE if not
 * - MALBOLGE_ERR_NULL_ARG if @a buffer is NULL
 */
int malbolge_is_likely_normalised_source(const char *buffer,
                                         unsigned long size);

/** Normalises the program source in @a buffer.
 *
 * For an explanation on normalistion, refer to
 * malbolge::normalise_source(InputIt first, InputIt last).
 * @param buffer Program source
 * @param size Size in bytes of @a buffer
 * @param new_size Normalised program size - always less than @a size, no data
 * is freed, but the one-past-the-end buffer element is set to null
 * @param fail_line If parsing fails, this is set to the line in the source
 * where the failure occurred.  Ignored if NULL
 * @param fail_column If the parsing fails, this is set to the column in the
 * source where the failure occurred.  Ignored if NULL
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a buffer or @a new_size is NULL
 * - MALBOLGE_ERR_PARSE_FAIL if the normalisation process failed
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_normalise_source(char *buffer,
                              unsigned long size,
                              unsigned long *new_size,
                              unsigned int *fail_line,
                              unsigned int *fail_column);

/** Denormalises the program source in @a buffer.
 *
 * For an explanation on normalistion, refer to
 * malbolge::denormalise_source(InputIt first, InputIt last).
 * @param buffer Program source
 * @param size Size in bytes of @a buffer
 * @param fail_column If the parsing fails, this is set to the column in the
 * source where the failure occurred.  Ignored if NULL
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a buffer is NULL
 * - MALBOLGE_ERR_PARSE_FAIL if the denormalisation process failed
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_denormalise_source(char *buffer,
                                unsigned long size,
                                unsigned int *fail_column);

/** Creates a malbolge::virtual memory instance and loads the program source
 * into it.
 *
 * If you do not pass the return pointer to malbolge_vcpu_run, it will need
 * manually freeing.
 *
 * @a buffer is not freed by this function.
 * @param buffer Program source
 * @param size Size in bytes of @a buffer
 * @param mode Program load normalised mode
 * @param fail_line If parsing fails, this is set to the line in the source
 * where the failure occurred.  Ignored if NULL
 * @param fail_column If the parsing fails, this is set to the column in the
 * source where the failure occurred.  Ignored if NULL
 * @return Malbolge virtual memory handle, or NULL if loading failed
 */
malbolge_virtual_memory malbolge_load_program(char *buffer,
                                              unsigned long size,
                                              malbolge_load_normalised_mode mode,
                                              unsigned int *fail_line,
                                              unsigned int *fail_column);

/** Frees the virtual memory returned from malbolge_load_program.
 *
 * This is only required if @a vmem is not going to be ran.
 * @param vmem Virtual memory to free
 */
void malbolge_free_virtual_memory(malbolge_virtual_memory vmem);

/** Creates a virtual CPU from the program in @a vmem.
 *
 * @param vmem Virtual memory handle returned from malbolge_load_program,
 * @a vmem is freed by this function
 * @return Malbolge virtual CPU handle, or NULL if @a vmem is NULL
 */
malbolge_virtual_cpu malbolge_create_vcpu(malbolge_virtual_memory vmem);

/** Synchronously stops and frees the vCPU created by
 *  malbolge_create_vcpu(malbolge_virtual_memory).
 *
 * If the program has not already stopped, this will stop it and cause the
 * vCPU's malbolge_vcpu_state_callback to be called.
 *
 * No-op if vcpu is NULL.
 * @param vcpu vCPU to free
 */
void malbolge_free_vcpu(malbolge_virtual_cpu vcpu);

/** Attach/bind/connect callbacks to @a vcpu.
 *
 * You can call this more than once to bind multiple callbacks.  The callback
 * addresses are stored separately so duplicates are ignored.
 * @note The callbacks are called from the vCPU's internal worker, it is the
 * caller's responsibility to manage thread safety within the callback
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param state_cb Callback called when the program state changes, or null to
 * skip
 * @param output_cb Callback called when the program emits output, or null to
 * skip
 * @param bp_cb Callback called when a breakpoint is hit in the program, or null
 * to skip
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_attach_callbacks(malbolge_virtual_cpu vcpu,
                                   malbolge_vcpu_state_callback state_cb,
                                   malbolge_vcpu_output_callback output_cb,
                                   malbolge_vcpu_breakpoint_hit_callback bp_cb);

/** Detach/unbind/disconnect callbacks from @a vcpu.
 *
 * The callback addresses are used to remove their entries, so this can be
 * called more than once if multiple callbacks were previously attached.
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param state_cb Callback called when the program state changes, or null to
 * skip
 * @param output_cb Callback called when the program emits output, or null to
 * skip
 * @param bp_cb Callback called when a breakpoint is hit in the program, or null
 * to skip
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_detach_callbacks(malbolge_virtual_cpu vcpu,
                                   malbolge_vcpu_state_callback state_cb,
                                   malbolge_vcpu_output_callback output_cb,
                                   malbolge_vcpu_breakpoint_hit_callback bp_cb);

/** Asynchronously run or resume the program loaded in @a vcpu.
 *
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if the vCPU is already in a stopped state
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_run(malbolge_virtual_cpu vcpu);

/** Asynchronously pauses the program.
 *
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if the vCPU is already in a stopped state
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_pause(malbolge_virtual_cpu vcpu);

/** Asynchronously advances the program by a single instruction.
 *
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if the vCPU is already in a stopped state
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_step(malbolge_virtual_cpu vcpu);

/** Asynchronsouly passes @a buffer to @a vcpu to use as user input.
 *
 * @a buffer is copied into the vCPU, so this can be called before @a vcpu is
 * ready to receive user input.  As the data is copied, you need to free
 * @a buffer manually (assuming it was malloc-ed).
 *
 * If the program was in a MALBOLGE_VCPU_WAITING_FOR_INPUT state, this will
 * resume it.
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param buffer Input data
 * @param size Size of @a buffer
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu or @a buffer is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_add_input(malbolge_virtual_cpu vcpu,
                            const char* buffer,
                            unsigned int size);

/** Adds a breakpoint.
 *
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param address vmem address to attach the breakpoint to
 * @param ignore_count Number times the breakpoint is hit before any
 * malbolge_vcpu_breakpoint_hit_callback callbacks are triggered
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_add_breakpoint(malbolge_virtual_cpu vcpu,
                                 unsigned int address,
                                 unsigned int ignore_count);

/** Removes a breakpoint at the given address.
 *
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param address vmem address to remove the breakpoint from
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_remove_breakpoint(malbolge_virtual_cpu vcpu,
                                    unsigned int address);

/** Asynchronously returns the value at a given vmem address.
 *
 * @note The callback is called from the vCPU's internal worker, it is the
 * caller's responsibility to manage thread safety within the callback
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param address vmem address, will wrap if same or larger than
 * math::ternary::max
 * @param cb Callback used to return the value
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu or @a cb is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_address_value(malbolge_virtual_cpu vcpu,
                                unsigned int address,
                                malbolge_vcpu_address_value_callback cb);

/** Asynchronously returns the address and/or value of a given register.
 *
 * @note The callback is called from the vCPU's internal worker, it is the
 * caller's responsibility to manage thread safety within the callback
 * @param vcpu vCPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param reg vCPU register to query
 * @param cb Callback used to return the value
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu or @a cb is NULL
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_register_value(malbolge_virtual_cpu vcpu,
                                 enum malbolge_vcpu_register reg,
                                 malbolge_vcpu_register_value_callback cb);
}
