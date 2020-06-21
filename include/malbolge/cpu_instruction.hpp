/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <array>
#include <algorithm>

namespace malbolge
{
/** Virtual machine processor instructions.
 *
 * Only these instructions are allowed during program load, once running any
 * non-whitespace ASCII not in this enum is considered a nop.
 */
namespace cpu_instruction
{
/** Instruction enumeration.
 */
enum type : char
{
    /** Sets the data pointer address to the value at the data pointer.
     */
    set_data_ptr = 'j',
    /** Sets the code pointer address to the value at the data pointer.
     */
    set_code_ptr = 'i',
    /** Rotate the data pointer value.
     */
    rotate = '*',
    /** Perform the ternary op on the data pointer value and contents of the A
     * register.
     */
    op = 'p',
    /** Read an ASCII value from stdin and store in the accumulator register.
     *
     * 10 (lf) is considered 'newline', and math::ternary::max is EOF.
     */
    read = '<',
    /** Writes the value in the accumulator register to stdout.
     *
     * 10 (lf) is considered 'newline'.
     */
    write = '/',
    /** Ends the program.
     */
    stop = 'v',
    /** Nop (no operation).
     *
     * During program execution, any non-instruction value is considered a nop,
     * but this is the only nop value allowed during program load.
     */
    nop = 'o',
};

/** An array of all the CPU instructions.
 *
 * Useful for iterating over all of them.
 */
constexpr auto all = std::array{
    set_data_ptr,
    set_code_ptr,
    rotate,
    op,
    read,
    write,
    stop,
    nop
};
}

/** True if @a instruction is a valid Malbolge CPU instruction.
 *
 * @tparam T Instruction type, must be implicitly convertible to char
 * @param instruction Instruction to test
 * @return True if valid
 */
template <typename T>
constexpr bool is_cpu_instruction(T instruction)
{
    return std::ranges::any_of(cpu_instruction::all, [&](auto i) {
        return i == instruction;
    });
}

/** The [min, max] interval of graphical characters in ASCII.
 */
constexpr auto graphical_ascii_range = std::pair<char, char>{33, 126};

/** True if @a c is within the graphical ASCII range [33, 126].
 *
 * @tparam T Character type, must be comparable to char
 * @param c Character to test
 * @return True if graphical ASCII
 */
template <typename T>
constexpr bool is_graphical_ascii(T c)
{
    return c >= graphical_ascii_range.first &&
           c <= graphical_ascii_range.second;
}
}

#endif // INSTRUCTIONS_HPP
