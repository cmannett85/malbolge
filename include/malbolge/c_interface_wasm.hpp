/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/c_interface.hpp"

/** @file
 * This provides WASM-specific functions that extend the C interface defined
 * in malbolge/c_interface.hpp.
 */

extern "C"
{
/** Run the program loading in @a vcpu.
 *
 * This is equivalent to
 * malbolge_vcpu_run(malbolge_virtual_cpu, malbolge_program_stopped, malbolge_program_waiting_for_input, int),
 * except the that <TT>use_cin</TT> argument is implicitly false and the
 * callbacks are implemented as <TT>EM_JS</TT> function calls.  The callbacks
 * use the JS Worker API to post messages back to the main thread where they can
 * be acted upon.
 *
 * For the program stopped message schema is:
 * @code
 * {
 *     "$schema": "http://json-schema.org/draft-04/schema#",
 *     "title": "Malbolge program stopped message",
 *     "type": "object",
 *     "required": ["cmd", "errorCode", "vcpu"],
 *     "properties": {
 *         "cmd": {
 *             "type": "string",
 *             "enum": ["malbolgeStopped"],
 *             "description": "Command type"
 *         },
 *         "errorCode": {
 *             "type": "integer",
 *             "maximum": 0,
 *             "description": "Negative error code that triggered the stop, or 0 if execution ended successfully",
 *         },
 *         "vcpu": {
 *             "type": "integer",
 *             "minimum": 0,
 *             "description": "malbolge_virtual_cpu pointer as provided by Emscripten"
 *         }
 *     }
 * }
 * @endcode
 *
 * The equivalent for the waiting-for-input message:
 * @code
 * {
 *     "$schema": "http://json-schema.org/draft-04/schema#",
 *     "title": "Malbolge waiting-for-input message",
 *     "type": "object",
 *     "required": ["cmd", "vcpu"],
 *     "properties": {
 *         "cmd": {
 *             "type": "string",
 *             "enum": ["malbolgeWaitingForInput"],
 *             "description": "Command type"
 *         },
 *         "vcpu": {
 *             "type": "integer",
 *             "minimum": 0,
 *             "description": "malbolge_virtual_cpu pointer as provided by Emscripten"
 *         }
 *     }
 * }
 * @endcode
 *
 * @param vcpu vCPU handle
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if vCPU execution initialisation fails
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_run_wasm(malbolge_virtual_cpu vcpu);
}
