/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/algorithm/trim.hpp"
#include "malbolge/cpu_instruction.hpp"
#include "malbolge/exception.hpp"

namespace malbolge
{
/** 'Normalises' a Malbolge program.
 *
 * In Malbolge the position of an instruction changes its meaning, this function
 * removes that feature i.e. normalises it, leaving only vCPU instructions.
 *
 * The advantage of normalisation is that it is much easier to generate (and
 * understand) a normalised program, and then add the mapping back in to run it.
 * The act of reinstating the mapping is 'denormalisation'.
 *
 * This function has to parse the input source and therefore can throw if
 * invalid characters are found, however be aware that this function changes the
 * source in-place so any errors found will @em not undo the existing changes.
 *
 * Normalisation can never result in a larger program, but will result in a
 * smaller one if whitespace is present in the input.
 * @tparam InputIt Input iterator type
 * @param first Source input begin iterator
 * @param last Source input one-past-the-end iterator
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt>
InputIt normalise_source(InputIt first, InputIt last)
{
#ifdef EMSCRIPTEN
    static_assert(!std::is_const_v<typename std::iterator_traits<InputIt>::value_type>,
#else
    static_assert(!std::is_const_v<std::iter_value_t<InputIt>>,
#endif
                  "InputIt must not be a const iterator");

    auto loc = source_location{};
    auto i = 0u;
    for (auto it = first; it != last; ++it) {
        if (std::isspace(*it)) {
            if (*it == '\n') {
                ++loc.line;
                loc.column = 1;
            } else {
                ++loc.column;
            }

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

        *first++ = *instr;
        ++loc.column;
        ++i;
    }

    return first;
}

/** Range-based overload.
 *
 * @tparam InputRange Input range type
 * @param source Source input range
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange>
typename InputRange::iterator normalise_source(InputRange& source)
{
    using std::begin;
    using std::end;

    return normalise_source(begin(source), end(source));
}

/** Same as normalise_source(InputRange&& source) but resizes @a source.
 *
 * @note This function requires @a source to conform to
 * <TT>SequenceContainer</TT> to support an <TT>erase(q1, q2)</TT> method
 * @tparam InputRange Input range type
 * @param source Source input range
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange>
void normalise_source_resize(InputRange& source)
{
    using std::end;

    auto it = normalise_source(source);
    source.erase(it, end(source));
}

/** 'Denormalises' Malbolge program from the 'normalised' input source.
 *
 * This function reinstates the initial mapping for a Mabolge program, so it
 * can be ran in the vCPU.
 *
 * This function has to parse the input source and therefore can throw if
 * invalid characters are found, however be aware that this function changes the
 * source in-place so any errors found will @em not undo the existing changes.
 *
 * Due to the prevalence of adding a newline as the last character of a file,
 * this function will ignore any contiguous quantity of whitespace characters
 * at the end of a file. As such denormalisation can result in output smaller
 * than the input.
 * @tparam InputIt Input iterator type
 * @param first Source input begin iterator
 * @param last Source input one-past-the-end iterator
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt>
InputIt denormalise_source(InputIt first, InputIt last)
{
#ifdef EMSCRIPTEN
    static_assert(!std::is_const_v<typename std::iterator_traits<InputIt>::value_type>,
#else
    static_assert(!std::is_const_v<std::iter_value_t<InputIt>>,
#endif
                  "InputIt must not be a const iterator");

    algorithm::trim_right(first, last, [](auto c) { return std::isspace(c); });

    constexpr auto map = std::array{
        std::array<char, 2>{cpu_instruction::rotate,         '\''},
        std::array<char, 2>{cpu_instruction::set_data_ptr,   '('},
        std::array<char, 2>{cpu_instruction::op,             '>'},
        std::array<char, 2>{cpu_instruction::nop,            'D'},
        std::array<char, 2>{cpu_instruction::stop,           'Q'},
        std::array<char, 2>{cpu_instruction::set_code_ptr,   'b'},
        std::array<char, 2>{cpu_instruction::write,          'c'},
        std::array<char, 2>{cpu_instruction::read,           'u'}
    };
    static_assert(map.size() == cpu_instruction::all.size(),
                  "CPU instruction list changed without updating normalisation map");

    for (auto i = 0u; first != last; ++first, ++i) {
        auto m_it = std::find_if(map.begin(), map.end(), [&](auto k) {
                return *first == k.front();
        });
        if (m_it == map.end()) {
            throw parse_exception{"Invalid instruction in program: " +
                                      std::to_string(static_cast<int>(*first)),
                                  source_location{1, i+1}};
        }

        const auto offset = static_cast<int>(i % cipher::size);
        const auto sub = static_cast<int>((*m_it).back()) - offset;

        if (sub < graphical_ascii_range.first) {
            *first = static_cast<char>(sub + cipher::size);
        } else {
            *first = static_cast<char>(sub);
        }
    }

    return first;
}

/** Range-based overload.
 *
 * @tparam InputRange Input range type
 * @param source Source input range
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange>
typename InputRange::iterator denormalise_source(InputRange& source)
{
    using std::begin;
    using std::end;

    return denormalise_source(begin(source), end(source));
}

/** Same as denormalise_source(InputRange&& source) but resizes @a source.
 *
 * @note This function requires @a source to conform to
 * <TT>SequenceContainer</TT> to support an <TT>erase(q1, q2)</TT> method
 * @tparam InputRange Input range type
 * @param source Source input range
 * @return One-past-the-end result iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange>
void denormalise_source_resize(InputRange& source)
{
    using std::end;

    auto it = denormalise_source(source);
    source.erase(it, end(source));
}

/** Returns true if the input source is likely to be normalised.
 *
 * This function reads the input source and returns true if it only contains
 * Malbolge vCPU instructions.  There is a @em very small chance that a
 * non-normalised program contains only vCPU instructions so do not use this
 * function as a critical check.
 *
 * Due to the prevalence of adding a newline as the last character of a file,
 * this function will ignore any contiguous quantity of whitespace characters
 * at the end of a file.
 * @note Will return true on an empty input
 * @tparam InputIt Input iterator type
 * @param first Source input begin iterator
 * @param last Source input one-past-the-end iterator
 * @return True if the input source is likely to be normalised
 */
template <typename InputIt>
bool is_likely_normalised_source(InputIt first, InputIt last)
{
    algorithm::trim_right(first, last, [](auto c) { return std::isspace(c); });
    for (; first != last; ++first) {
        if (!is_cpu_instruction(*first)) {
            return false;
        }
    }

    return true;
}

/** Range-based overload.
 *
 * @note Will return true on an empty input
 * @tparam InputRange Input range type
 * @param source Source input range
 * @return True if the input source is likely to be normalised
 */
template <typename InputRange>
bool is_likely_normalised_source(InputRange&& source)
{
    using std::begin;
    using std::end;

    return is_likely_normalised_source(begin(source), end(source));
}
}
