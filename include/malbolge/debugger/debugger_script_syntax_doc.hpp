/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

/** @page debugger_script_syntax Debugger Script Syntax
 * @tableofcontents
 * 
 * Primitive debugging is supported through the use of a debugging script
 * specified by the <TT>--debugger-script flag</TT>, for example:
 * @code{.unparsed}
 * malbolge -ll --debugger-script my_prog.dbg my_prog.mal
 * @endcode
 * The file extension is not relevant, it can be anything (or nothing). The
 * script is essentially a list of commands for the debugger, it's syntax and
 * supported functions are listed below.
 * 
 * Examples can be found in /test/programs.
 * 
 * @section syntax Syntax
 * - Whitespace is ignored, commands are ended by a <TT>;</TT>
 * - Arguments are held within brackets and are named, bound by <TT>=</TT>
 * - Arguments that have default values do not need to be specified
 * - Arguments are separated by <TT>,</TT>
 * - Return values are printed to the error stream (same as logging), but
 *   prefixed with <TT>\<Timestamp\>[DBGR]: </TT>
 * - Comments can start anywhere, signified by <TT>//</TT>, and continue until
 *   the end of the line
 * 
 * @section types Argument types
 * <b><TT>uint</TT></b>\n
 * Unsigned integer. Base-10 unless prefixed with <TT>0x</TT> which means
 * base-16, or <TT>0</TT> which means octal.
 * 
 * <b><TT>ternary</TT></b>\n
 * 10-trit unsigned ternary. Base-10 input unless prefixed with <TT>t</TT> which
 * means base-3.
 * 
 * <b><TT>reg</TT></b>\n
 * vCPU register, <TT>A</TT>, <TT>C</TT>, or <TT>D</TT> (case-sensitive).
 * 
 * <b><TT>string</TT></b>\n
 * ACSII string, must be surrounded by <TT>""</TT> and standard escaping rules
 * apply. Hex and octal escaping supported too.
 * 
 * @section functions Functions
 * @subsection add_breakpoint
 * Adds a breakpoint at the given address. If one already exists at
 * <TT>address</TT> then the previous one is replaced. If there are any
 * breakpoints listed in a script, then at least one must appear before the run
 * command.
 * @code{.unparsed}
 * add_breakpoint(address=9, ignore_count=2);
 * @endcode
 * @subsubsection add_breakpoint_args Arguments
 * <b><TT>address = uint/ternary [Required]</TT></b>\n
 * Memory location in the virtual memory block to attach the breakpoint.
 * 
 * <b><TT>ignore_count [Defaults to 0]</TT></b>\n
 * Number of times <TT>address</TT> is hit before execution is paused.
 * 
 * @subsection remove_breakpoint
 * Removes a breakpoint at the given address. If no breakpoint exists at
 * <TT>address</TT> then it is a no-op.
 * @code{.unparsed}
 * remove_breakpoint(address=9);
 * @endcode
 * @subsubsection remove_breakpoint_args Arguments
 * <b><TT>address = uint/ternary [Required]</TT></b>\n
 * Memory location in the virtual memory block to remove a breakpoint from.
 * 
 * @subsection run
 * Begins execution of the program. If <TT>max_runtime_ms</TT> is non-zero then
 * program execution is stopped (@em stopped, not paused) after the number of
 * milliseconds requested unless a breakpoint is hit first. Subsequent commands
 * in the script are not read until a breakpoint is hit or the program ends.
 * 
 * There can only be a single <TT>run</TT> command in a script, use
 * <TT>resume</TT> to continue execution after a breakpoint is hit.
 * @code{.unparsed}
 * run();
 * @endcode
 * @subsubsection run_args Arguments
 * <b><TT>max_runtime_ms = uint/ternary [Defaults to 0, which represents
 * infinite runtime]</TT></b>\n
 * Number of milliseconds the program is allowed to run without hitting a
 * breakpoint.
 * 
 * @subsection address_value
 * Returns the value at the given address. Although not a requirement, it is
 * generally only meaningful to call this function when the program is paused.
 * @code{.unparsed}
 * address_value(address=9);
 * @endcode
 * @subsubsection address_value_args Arguments
 * <b><TT>address = uint/ternary [Required]</TT></b>\n
 * Memory location in the virtual memory block to read.
 * @subsubsection address_value_example Example output
 * @code{.unparsed}
 * 2020-12-19 11:40:58.696620673[DBGR]: address_value(address={d:9, t:0000000100}); = {d:125, t:0000011122}
 * @endcode
 * 
 * @subsection register_value
 * Returns the value at the given register. Although not a requirement, it is
 * generally only meaningful to call this function when the program is paused.
 *
 * If the register is <TT>C</TT> or <TT>D</TT>, then the register contains an
 * address - this is printed along with the value at the address in virtual
 * memory.
 * @code{.unparsed}
 * register_value(reg=A);
 * @endcode
 * @subsubsection register_value_args Arguments
 * <b><TT>reg = reg [Required]</TT></b>\n
 * Memory location in the virtual memory block to read.
 * @subsubsection register_value_example Example output
 * @code{.unparsed}
 * 2020-12-19 11:40:58.696758245[DBGR]: register_value(reg=C); = {{d:9, t:0000000100}, {d:125, t:0000011122}}
 * @endcode
 * 
 * @subsection step
 * In a paused state, this will advance program execution by a single
 * instruction. Script parsing will fail if this function is before a
 * <TT>run</TT>.
 * @code{.unparsed}
 * step();
 * @endcode
 * 
 * @subsection resume
 * In a paused state, this will resume program execution. Script parsing will
 * fail if this function is before a <TT>run</TT>.
 * @code{.unparsed}
 * resume();
 * @endcode
 * 
 * @subsection on_input
 * Adds a string onto the input data queue. When the program requests input data
 * (i.e. from <TT>cin</TT>), then the next string in this queue is passed to it
 * or the program will block waiting for input. This allows you to 'pre-load'
 * input data before starting program execution.
 * @code{.unparsed}
 * on_input(data="Hello world!");
 * @endcode
 * @subsubsection on_input_args Arguments
 * <b><TT>data = string [Required]</TT></b>\n
 * Program input.
 */
