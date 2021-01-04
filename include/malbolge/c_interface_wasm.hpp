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
 * malbolge_vcpu_run(malbolge_virtual_cpu), except that it sets up the
 * callbacks internally as they are implemented as <TT>EM_JS</TT> function
 * calls.  The callbacks use the JS Worker API to post messages back to the main
 * thread where they can be acted upon.
 *
 * The JSON schema for the vCPU state message:
 * @code
 * {
 *     "$schema": "http://json-schema.org/draft-04/schema#",
 *     "title": "Malbolge vCPU state message",
 *     "type": "object",
 *     "required": ["cmd", "state", "errorCode", "vcpu"],
 *     "properties": {
 *         "cmd": {
 *             "type": "string",
 *             "enum": ["malbolgevCPUState"],
 *             "description": "Command type"
 *         },
 *         "state": {
 *             "type": "integer",
 *             "minimum": 0,
 *             "description": "Equivalent to malbolge_vcpu_execution_state"
 *         },
 *         "errorCode": {
 *             "type": "integer",
 *             "maximum": 0,
 *             "description": "Negative error code that triggered the stop, or 0 if execution ended successfully"
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
 * The breakpoint hit message:
 * @code
 * {
 *     "$schema": "http://json-schema.org/draft-04/schema#",
 *     "title": "Malbolge breakpoint hit message",
 *     "type": "object",
 *     "required": ["cmd", "address", "vcpu"],
 *     "properties": {
 *         "cmd": {
 *             "type": "string",
 *             "enum": ["malbolgeBreakpoint"],
 *             "description": "Command type"
 *         },
 *         "address": {
 *             "type": "integer",
 *             "maximum": 0,
 *             "maximum": 59048,
 *             "description": "Virtual memory address that triggered the breakpoint"
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
 * The program output message seems like it shouldn't be necessary as we could
 * just write to cout and the user-specified JS stdOut handler can process it,
 * but because Emscripten sees cout as a way of adding logging to the browser's
 * console it does weird things with newlines related to flushing.  This is fine
 * for logging, but does not work well with program output.
 *
 * As sending this message for every character is extremely inefficient, it is
 * sent on every 10 characters and vCPU state change (if any characters are
 * waiting in the internal buffer).
 * @code
 * {
 *     "$schema": "http://json-schema.org/draft-04/schema#",
 *     "title": "Malbolge program output message",
 *     "type": "object",
 *     "required": ["cmd", "data", "vcpu"],
 *     "properties": {
 *         "cmd": {
 *             "type": "string",
 *             "enum": ["malbolgeOutput"],
 *             "description": "Command type"
 *         },
 *         "data": {
 *             "type": "string",
 *             "maxLength": 10,
 *             "description": "Program output, up to 10 chars max"
 *         },
 *         "vcpu": {
 *             "type": "integer",
 *             "minimum": 0,
 *             "description": "malbolge_virtual_cpu pointer as provided by Emscripten"
 *         }
 *     }
 * }
 * @endcode
 * @param vcpu vCPU handle
 * @return
 * - MALBOLGE_ERR_SUCCESS for success
 * - MALBOLGE_ERR_NULL_ARG if @a vcpu is NULL
 * - MALBOLGE_ERR_EXECUTION_FAIL if vCPU execution initialisation fails
 * - MALBOLGE_ERR_UNKNOWN if an unknown failure occurs
 */
int malbolge_vcpu_run_wasm(malbolge_virtual_cpu vcpu);
}
