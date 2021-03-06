/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/virtual_memory.hpp"
#include "malbolge/algorithm/remove_from_range.hpp"
#include "malbolge/log.hpp"
#include "malbolge/normalise.hpp"

#include <filesystem>

namespace malbolge
{
/** Program load normalised modes.
 */
enum class load_normalised_mode
{
    AUTO,       ///< Automatically detect if normalised, uses
                ///< is_likely_normalised_source(InputIt, InputIt)
    ON,         ///< Force load as normalised
    OFF,        ///< Force load as non-normalised
    NUM_MODES   ///< Number of normalisation modes
};

/** Textual streaming operator.
 *
 * @param stream Output stream
 * @param mode Program load normalised mode
 * @return @a stream
 */
std::ostream& operator<<(std::ostream& stream, load_normalised_mode mode);

/** Loads the program data between @a first and @a last.
 *
 * The data is modified in place, so the iterators must not be const.
 * @note <TT>std::iterator_traits<InputIt>::value_type</TT> needs to be
 * explicitly convertible to <TT>cpu_instruction::type</TT>.
 * @tparam InputIt Input iterator type
 * @param first Iterator to the first element
 * @param last Iterator to the one-past-the-end element
 * @param mode Program load normalised mode
 * @return Virtual memory image with the program at the start
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt>
[[nodiscard]]
virtual_memory load(InputIt first,
                    InputIt last,
                    load_normalised_mode mode = load_normalised_mode::AUTO)
{
#ifdef EMSCRIPTEN
    static_assert(!std::is_const_v<typename std::iterator_traits<InputIt>::value_type>,
#else
    static_assert(!std::is_const_v<std::iter_value_t<InputIt>>,

#endif
                  "InputIt must not be a const iterator");

    auto normalised = false;
    switch (mode) {
    case load_normalised_mode::AUTO:
        normalised = is_likely_normalised_source(first, last);
        break;
    case load_normalised_mode::ON:
        normalised = true;
        break;
    default:
        break;
    }

    if (normalised) {
        // If the source is normalised, we need to denormalise before executing.
        // However the act of denormalising checks the syntax, so we can skip
        // that test here
        last = denormalise_source(first, last);
    } else {
        auto loc = source_location{};
        auto i = 0u;
        for (auto it = first; it != last;) {
            if (std::isspace(*it)) {
                if (*it == '\n') {
                    ++loc.line;
                    loc.column = 1;
                } else {
                    ++loc.column;
                }

                last = algorithm::remove_from_range(it, last);
                continue;
            }

            auto instr = pre_cipher_instruction(*it, i);
            if (!instr) {
                throw parse_exception{"Non-whitespace character must be graphical "
                                          "ASCII: " +
                                          std::to_string(static_cast<int>(*it)),
                                      loc};
            }

            if (!is_cpu_instruction(*instr)) {
                throw parse_exception{"Invalid instruction in program: " +
                                          std::to_string(static_cast<int>(*instr)),
                                      loc};
            }
            ++loc.column;
            ++it;
            ++i;
        }
    }

    log::print(log::DEBUG, "Loaded size: ", std::distance(first, last));

    return virtual_memory(first, last);
}

/** Loads the program data in @a range.
 *
 * This is equivalent to
 * @code
 * load(std::begin(range), std::end(range));
 * @endcode
 * The data is modified in place, so the created iterators must not be const.
 *
 * @note <TT>R:value_type</TT> needs to be explicitly convertible to
 * <TT>cpu_instruction::type</TT>.
 * @tparam Range Range type
 * @param range Range instance
 * @param mode Program load normalised mode
 * @return Virtual memory image with the program at the start
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename R,
          std::enable_if_t<!std::is_same_v<std::decay_t<R>,
                                           std::filesystem::path>,
                           int> = 0>
[[nodiscard]]
virtual_memory load(R&& range,
                    load_normalised_mode mode = load_normalised_mode::AUTO)
{
    using std::begin;
    using std::end;

    log::print(log::INFO, "Loading file from string");
    return load(begin(range), end(range), mode);
}

/** Loads the program data read from @a path.
 *
 * @param path Path to text file containing the program
 * @param mode Program load normalised mode
 * @return Virtual memory image with the program at the start
 * @exception parse_exception Thrown if the program cannot be read or contains
 * errors
 */
[[nodiscard]]
virtual_memory load(const std::filesystem::path& path,
                    load_normalised_mode mode = load_normalised_mode::AUTO);

/** Loads the program data from std::cin.
 *
 * This is used for 'piping' data in from a terminal.
 * @param mode Program load normalised mode
 * @return Virtual memory image with the program at the start
 * @exception parse_exception Thrown if the program contains errors
 */
[[nodiscard]]
virtual_memory load_from_cin(load_normalised_mode mode = load_normalised_mode::AUTO);
}
