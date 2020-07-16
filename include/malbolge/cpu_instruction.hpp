/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <array>
#include <algorithm>

namespace malbolge
{
/** Namespace for virtual machine processor instructions.
 */
namespace cpu_instruction
{
/** Instruction enumeration.
 * 
 * Only these instructions are allowed during program load, once running any
 * non-whitespace ASCII not in this enum is considered a nop.
 */
enum type : char
{
    /** Sets the data pointer address to the value at the data pointer.
     *
     * @note The read and write operations are switched between the
     * specification and the reference implementation.  The reference
     * implementation's version is used here
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
    read = '/',
    /** Writes the value in the accumulator register to stdout.
     *
     * 10 (lf) is considered 'newline'.
     */
    write = '<',
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
inline constexpr bool is_cpu_instruction(T instruction)
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
inline constexpr bool is_graphical_ascii(T c)
{
    return c >= graphical_ascii_range.first &&
           c <= graphical_ascii_range.second;
}

/** Namespace for cipher functions and constants.
 */
namespace cipher
{
/** Cipher size.
 */
constexpr auto size = graphical_ascii_range.second -
                      graphical_ascii_range.first;

/** Returns the pre-ciphered character at @a index.
 *
 * @param index Index into pre-cipher array
 * @return Character in the pre-cipher at @a index
 */
char pre(std::size_t index);

/** Returns the post-ciphered character at @a index.
 *
 * @param index Index into post-cipher array
 * @return Character in the post-cipher at @a index
 */
char post(std::size_t index);
}

/** Performs a pre-instruction cipher on @a input.
 *
 * @tparam T Input character type
 * @param input Input character
 * @param index Index of the character in the program data
 * @return The ciphered instruction, or an empty optional if @a input is not
 * within in the graphical ASCII range
 */
template <typename T>
std::optional<char> pre_cipher_instruction(T input, std::size_t index)
{
    if (!is_graphical_ascii(input)) {
        return {};
    }

    const auto i = (input - graphical_ascii_range.first + index) %
                   (cipher::size+1);
    return cipher::pre(static_cast<std::size_t>(i));
}

/** Performs a post-instruction cipher on @a input.
 *
 * @tparam T Input character type
 * @param input Input character
 * @return The ciphered instruction, or an empty optional if @a input is not
 * within in the graphical ASCII range
 */
template <typename T>
std::optional<char> post_cipher_instruction(T input)
{
    if (!is_graphical_ascii(input)) {
        return {};
    }

    input -= graphical_ascii_range.first;
    return cipher::post(static_cast<std::size_t>(input));
}
}
