/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#ifndef LOADER_MALBOLGE_HPP
#define LOADER_MALBOLGE_HPP

#include "malbolge/virtual_memory.hpp"
#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"

#include "test_helpers.hpp"

#include <filesystem>
#include <ranges>
#include <optional>
#include <vector>

namespace malbolge
{
namespace detail
{
using namespace std::string_literals;

template <typename InputIt>
virtual_memory load_impl(InputIt first, InputIt last)
{
    // It would be nice to be able to use ranges here, but there didn't seem
    // to be a way of calculating the final index of a character reliably.
    // Whereas a simple for-loop does it quite effectively
    const auto is_whitespace = [](auto b) {
        return std::isspace(b);
    };
    last = std::remove_if(first, last, is_whitespace);

    auto i = 0u;
    for (auto it = first; it != last; ++it, ++i) {
        auto& b = *it;
        b = pre_cipher_instruction(b, i);
        if (!is_cpu_instruction(b)) {
            throw std::invalid_argument{"Invalid instruction in program: "s +
                                        std::to_string(static_cast<int>(b))};
        }
    }

    return virtual_memory(first, last);
}
}

/** Loads the program data between @a first and @a last.
 *
 * @note <TT>std::iterator_traits<InputIt>::value_type</TT> needs to be
 * explicitly convertible <TT>cpu_instruction::type</TT>.
 * @tparam InputIt Input iterator type
 * @param first Iterator to the first element
 * @param last Iterator to the one-past-the-end element
 * @return Virtual memory image with the program at the start
 * @throw std::invalid_argument Thrown if the program contains errors
 */
template <typename InputIt>
virtual_memory load(InputIt first, InputIt last)
{
    using namespace std::string_literals;

    try {
        return detail::load_impl(std::move(first), std::move(last));
    } catch (std::exception& e) {
        throw std::invalid_argument{"Failed to load program: "s + e.what()};
    }
}

/** Loads the program data in @a range.
 *
 * This is equivalent to
 * @code
 * load(std::begin(range), std::end(range));
 * @endcode
 *
 * @note <TT>R:value_type</TT> needs to be explicitly convertible
 * <TT>cpu_instruction::type</TT>.
 * @tparam Range Range type
 * @param range Range instance
 * @return Virtual memory image with the program at the start
 * @throw std::invalid_argument Thrown if the program contains errors
 */
template <typename R>
virtual_memory load(R&& range)
{
    return load(std::begin(range), std::end(range));
}

/** Loads the program data read from @a path.
 *
 * @param path Path to text file containing the program
 * @return Virtual memory image with the program at the start
 * @throw std::invalid_argument Thrown if the program cannot be read or contains
 * errors
 */
virtual_memory load(const std::filesystem::path& path);

/** Loads the program data from std::cin.
 *
 * This is used for 'piping' data in from a terminal.
 * @return Virtual memory image with the program at the start
 * @throw std::invalid_argument Thrown if the program contains errors
 */
virtual_memory load_from_cin();
}

#endif // LOADER_MALBOLGE_HPP
