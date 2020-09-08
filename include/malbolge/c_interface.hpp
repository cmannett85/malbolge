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

/** Function pointer signature for the program stopped callback.
 *
 * Args:
 * -# The negative error code that triggered the stop, or 0 if execution ended
 * successfully
 * -# vCPU handle that has stopped.  Note that the handle has already freed
 * when this function is called
 */
typedef void (*malbolge_program_stopped)(int, malbolge_virtual_cpu);

/** Function pointer signature for the waiting for user input callback.
 *
 * Args:
 * -# vCPU handle that is waiting
 */
typedef void (*malbolge_program_waiting_for_input)(malbolge_virtual_cpu);

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
 * @return 0 for success, a negative error code in case of failure
 */
int malbolge_set_log_level(unsigned int level);

/** Returns malbolge::version_string.
 *
 * The return is a string literal and so must not be user freed.
 * @return The full version string.
 */
const char *malbolge_version();

/** Creates a malbolge::virtual memory instance and loads the program source
 * into it.
 *
 * If you do not pass the return pointer to malbolge_vcpu_run, it will need
 * manually freeing.
 *
 * @a buffer is not freed by this function.
 * @param buffer Program source
 * @param size Size in bytes of @a data
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

/** Frees the virtual memory returned from malbolge_load_program.
 *
 * This is only required if @a vmem is not going to be ran.
 * @param vmem Virtual memory to free
 */
void malbolge_free_virtual_memory(malbolge_virtual_memory vmem);

/** Creates a virtual CPU and begins asynchronously executing the code in
 * @a vmem.
 *
 * @param vmem Virtual memory handle returned from malbolge_load_program,
 * @a vmem is freed by this function.
 * @param stopped_cb Callback called when the execution stops
 * @param waiting_cb Callback called when the program is expecting user input
 * @param use_cin If true, user input is extracted from cin
 * @return Malbolge virtual CPU handle, or NULL if early checks failed.  This is
 * freed when execution stops.
 */
malbolge_virtual_cpu malbolge_vcpu_run(malbolge_virtual_memory vmem,
                                       malbolge_program_stopped stopped_cb,
                                       malbolge_program_waiting_for_input waiting_cb,
                                       int use_cin);

/** Asynchronously stops the virtual CPU.
 *
 * The stopped callback assigned to @a vcpu is called.
 * @param vcpu Virtual CPU handle returned from malbolge_vcpu_run
 */
void malbolge_vcpu_stop(malbolge_virtual_cpu vcpu);

/** Passes @a buffer to @a vcpu to use as user input.
 *
 * This request is ignored if @a vcpu was created with the <TT>use_cin</TT>
 * argument set to true.
 *
 * @a buffer is copied into an internal queue, so this can be called before
 * @a vcpu is ready to receive user input.  As the data is copied, you need to
 * free @a buffer manually (assuming it was malloc-ed).
 * @param vcpu Virtual CPU handle returned from malbolge_vcpu_run
 * @param buffer Input data
 * @param size Size of @a buffer
 * @return 0 for success, a negative error code in case of failure
 */
int malbolge_vcpu_input(malbolge_virtual_cpu vcpu,
                        const char* buffer,
                        unsigned int size);
}
