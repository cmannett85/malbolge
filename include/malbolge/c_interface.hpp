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

/** Opaque handle for a debugger.
 */
typedef void* malbolge_debugger;

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
    MALBOLGE_ERR_CIN_OR_STOPPED         = -0x1005, ///< vCPU input requested but cin was specified,
                                                   ///<  or program stopped

    MALBOLGE_ERR_DBG_WRONG_STATE        = -0x1100, ///< Debugger operation whilst program
                                                   ///< is in wrong state
};

/** vCPU register identifiers for the debugger.
 *
 * C equivalent to malbolge::debugger::vcpu_register::id.
 */
enum malbolge_debugger_vcpu_register_id
{
    MALBOLGE_DBG_REGISTER_A,    ///< Accumulator
    MALBOLGE_DBG_REGISTER_C,    ///< Code pointer
    MALBOLGE_DBG_REGISTER_D,    ///< Data pointer
    MALBOLGE_DBG_REGISTER_MAX,  ///< Number of registers
};

/** Function pointer signature for the program stopped callback.
 *
 * @param error_code The negative error code that triggered the stop, or
 * MALBOLGE_ERR_SUCCESS if execution ended successfully
 * @param vcpu vCPU handle that has stopped.  Note that the handle has already
 * freed when this function is called
 */
typedef void (*malbolge_program_stopped)(int error_code,
                                         malbolge_virtual_cpu vcpu);

/** Function pointer signature for the waiting for user input callback.
 *
 * @param vcpu vCPU handle that is waiting
 */
typedef void (*malbolge_program_waiting_for_input)(malbolge_virtual_cpu vcpu);

/** Function pointer signature for a callback fired when a breakpoint is hit.
 *
 * @param address Address the breakpoint resides on
 * @param reg vCPU register that triggered the breakpoint
 * @return
 * - MALBOLGE_ERR_TRUE to stop execution
 * - MALBOLGE_ERR_FALSE to continue execution
 */
typedef int (*malbolge_debugger_breakpoint_callback)(
    unsigned int address,
    malbolge_debugger_vcpu_register_id reg);

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
 * @param fail_line If parsing fails, this is set to the line in the source
 * where the failure occurred.  Ignored if NULL
 * @param fail_column If the parsing fails, this is set to the column in the
 * source where the failure occurred.  Ignored if NULL
 * @return Malbolge virtual memory handle, or NULL if loading failed
 */
malbolge_virtual_memory malbolge_load_program(char *buffer,
                                              unsigned long size,
                                              unsigned int *fail_line,
                                              unsigned int *fail_column);

/** Same as malbolge_load_program(char*, unsigned long, unsigned int*, unsigned int*)
 * but denormalises the input before execution.
 *
 * If you do not pass the return pointer to malbolge_vcpu_run, it will need
 * manually freeing.
 *
 * @a buffer is not freed by this function.
 * @param buffer Program source
 * @param size Size in bytes of @a buffer
 * @param fail_line If parsing fails, this is set to the line in the source
 * where the failure occurred.  Ignored if NULL
 * @param fail_column If the parsing fails, this is set to the column in the
 * source where the failure occurred.  Ignored if NULL
 * @return Malbolge virtual memory handle, or NULL if loading failed
 */
malbolge_virtual_memory
malbolge_load_normalised_program(char *buffer,
                                 unsigned long size,
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

/** Frees the vCPU created by malbolge_create_vcpu(malbolge_virtual_memory).
 *
 * No-op if vcpu is NULL.
 * @param vcpu vCPU to free
 */
void malbolge_free_vcpu(malbolge_virtual_cpu vcpu);

/** Run the program loading in @a vcpu.
 *
 * @param vcpu vCPU handle
 * @param stopped_cb Callback called when the execution stops
 * @param waiting_cb Callback called when the program is expecting user input
 * @param use_cin If true, user input is extracted from cin.  Otherwise input
 * is passed in using malbolge_vcpu_input.
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if vCPU execution initialisation fails
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_run(malbolge_virtual_cpu vcpu,
                      malbolge_program_stopped stopped_cb,
                      malbolge_program_waiting_for_input waiting_cb,
                      int use_cin);

/** Asynchronously stops the virtual CPU.
 *
 * The stopped callback assigned to @a vcpu is called.
 * @param vcpu Virtual CPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 */
int malbolge_vcpu_stop(malbolge_virtual_cpu vcpu);

/** Passes @a buffer to @a vcpu to use as user input.
 *
 * This request is ignored if @a vcpu was created with the <TT>use_cin</TT>
 * argument set to true.
 *
 * @a buffer is copied into an internal queue, so this can be called before
 * @a vcpu is ready to receive user input.  As the data is copied, you need to
 * free @a buffer manually (assuming it was malloc-ed).
 * @param vcpu Virtual CPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @param buffer Input data
 * @param size Size of @a buffer
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu or @a buffer is NULL
 * - MALBOLGE_ERR_CIN_OR_STOPPED if @a vcpu is set to use cin, or already
 *   stopped
 */
int malbolge_vcpu_input(malbolge_virtual_cpu vcpu,
                        const char* buffer,
                        unsigned int size);

/** Attach a debugger to a vCPU.
 *
 * See malbolge::debugger::client_control for more information.
 * @param vcpu Virtual CPU handle returned from
 * malbolge_create_vcpu(malbolge_virtual_memory)
 * @return Debugger handle, or NULL if @a vcpu is NULL or the attachment failed
 */
malbolge_debugger malbolge_debugger_attach(malbolge_virtual_cpu vcpu);

/** Pause the debugged program.
 *
 * See malbolge::debugger::client_control::pause() for more information.
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 * - MALBOLGE_ERR_DBG_WRONG_STATE if program is stopped or not started
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_debugger_pause(malbolge_debugger debugger);

/** Step the debugged program.
 *
 * See malbolge::debugger::client_control::step() for more information.
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 * - MALBOLGE_ERR_DBG_WRONG_STATE if program is not in a paused state
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_debugger_step(malbolge_debugger debugger);

/** Resume the debugged program.
 *
 * See malbolge::debugger::client_control::resume() for more information.
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 * - MALBOLGE_ERR_DBG_WRONG_STATE if program is stopped or not started
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_debugger_resume(malbolge_debugger debugger);

/** Returns the value at a given vmem address.
 *
 * See malbolge::debugger::client_control::address_value(math::ternary) const
 * for more information.
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @param address vmem address, will wrap if same or larger than
 * math::ternary::max
 * @return If positive then the value at vmem @a address.  Otherwise:
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 * - MALBOLGE_ERR_DBG_WRONG_STATE if program is running
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_debugger_address_value(malbolge_debugger debugger,
                                    unsigned int address);

/** Returns the address and/or value of a given register.
 *
 * See
 * malbolge::debugger::client_control::register_value(vcpu_register::id) const
 * for more information.
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @param reg vCPU register to query
 * @param address The value pointed at by the register, or NULL if querying the
 * A register.  Ignored if the input is NULL
 * @return If positive then the register value.  Otherwise:
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 * - MALBOLGE_ERR_DBG_WRONG_STATE if program is running
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_debugger_register_value(malbolge_debugger debugger,
                                     enum malbolge_debugger_vcpu_register_id reg,
                                     unsigned int** address);

/** Adds a breakpoint.
 *
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @param address vmem address to attach the breakpoint to
 * @param cb Callback fired when the breakpoint is hit, or NULL to use the
 * default (malbolge::debugger::client_control::breakpoint::default_callback)
 * @param ignore_count Number times the breakpoint is hit before the callback is
 * triggered
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 */
int malbolge_debugger_add_breakpoint(malbolge_debugger debugger,
                                     unsigned int address,
                                     malbolge_debugger_breakpoint_callback cb,
                                     unsigned int ignore_count);

/** Removes a breakpoint at the given address.
 *
 * @param debugger Debugger handle returned from
 * malbolge_debugger_attach(malbolge_virtual_cpu)
 * @param address vmem address to remove the breakpoint from
 * @return
 * - MALBOLGE_ERR_TRUE if a breakpoint was removed
 * - MALBOLGE_ERR_FALSE if no breakpoint was at @a address
 * - MALBOLGE_ERR_NULL_ARG if @a debugger is NULL
 */
int malbolge_debugger_remove_breakpoint(malbolge_debugger debugger,
                                        unsigned int address);
}
