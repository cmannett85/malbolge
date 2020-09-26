/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

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
 * Denormalisation will result in a program the same size as the input source.
 * @tparam InputIt Input iterator type
 * @param first Source input begin iterator
 * @param last Source input one-past-the-end iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt>
void denormalise_source(InputIt first, InputIt last)
{
#ifdef EMSCRIPTEN
    static_assert(!std::is_const_v<typename std::iterator_traits<InputIt>::value_type>,
#else
    static_assert(!std::is_const_v<std::iter_value_t<InputIt>>,
#endif
                  "InputIt must not be a const iterator");

    constexpr auto map = std::array{
        std::array{'*', '\''},
        std::array{'j', '('},
        std::array{'p', '>'},
        std::array{'o', 'D'},
        std::array{'v', 'Q'},
        std::array{'i', 'b'},
        std::array{'<', 'c'},
        std::array{'/', 'u'}
    };

    for (auto i = 0u; first != last; ++first, ++i) {
#ifdef EMSCRIPTEN
        using std::begin;
        using std::end;
        auto m_it = std::find_if(begin(map), end(map), [&](auto k) {
#else
        auto m_it = std::ranges::find_if(map, [&](auto k) {
#endif
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
}

/** Range-based overload.
 *
 * @tparam InputRange Input range type
 * @param source Source input range
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange>
void denormalise_source(InputRange&& source)
{
    using std::begin;
    using std::end;

    denormalise_source(begin(source), end(source));
}
}
