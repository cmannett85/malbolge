/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/cpu_instruction.hpp"
#include "malbolge/exception.hpp"

namespace malbolge
{
/** Returns a 'normalised' Malbolge program from the input source.
 *
 * In Malbolge the position of an instruction changes its meaning, this function
 * removes that feature i.e. normalises it, leaving only vCPU instructions.
 *
 * The advantage of normalisation is that it is much easier to generate (and
 * understand) a normalised program, and then add the mapping back in to run it.
 * The act of reinstating the mapping is 'denormalisation'.
 *
 * This function has to parse the input source and therefore can throw if
 * invalid characters are found.  Unlike the load functions, this does @em not
 * modify the input, it creates a copy.
 * @tparam InputIt Input iterator type
 * @tparam OutputIt Output iterator type
 * @param s_begin Source input begin iterator
 * @param s_end Source input one-past-the-end iterator
 * @param d_begin Destination begin iterator
 * @return One-past-the-end destination iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt, typename OutputIt>
OutputIt normalise_source(InputIt s_begin, InputIt s_end, OutputIt d_begin)
{
    auto loc = source_location{};
    auto i = 0u;
    for (auto it = s_begin; it != s_end; ++it) {
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

        *d_begin++ = *instr;
        ++loc.column;
        ++i;
    }

    return d_begin;
}

/** Range-based overload.
 *
 * @tparam InputRange Input range type
 * @tparam OutputIt Output iterator type
 * @param source Source input range
 * @param d_begin Destination begin iterator
 * @return One-past-the-end destination iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange, typename OutputIt>
OutputIt normalise_source(InputRange&& source, OutputIt d_begin)
{
    using std::begin;
    using std::end;

    return normalise_source(begin(source), end(source), std::move(d_begin));
}

/** std::string overload.
 *
 * @param source Program source
 * @return Normalised version of @a source
 * @exception parse_exception Thrown if the program contains errors
 */
std::string normalise_source(const std::string& source);

/** Returns a 'denormalised' Malbolge program from the 'normalised' input
 * source.
 *
 * This function reinstates the initial mapping for a Mabolge program, so it
 * can be ran in the vCPU.
 *
 * This function has to parse the input source and therefore can throw if
 * invalid characters are found.  Unlike the load functions, this does @em not
 * modify the input, it creates a copy.
 * @tparam InputIt Input iterator type
 * @tparam OutputIt Output iterator type
 * @param s_begin Source input begin iterator
 * @param s_end Source input one-past-the-end iterator
 * @param d_begin Destination begin iterator
 * @return One-past-the-end destination iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputIt, typename OutputIt>
OutputIt denormalise_source(InputIt s_begin, InputIt s_end, OutputIt d_begin)
{
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

    auto loc = source_location{};
    auto i = 0u;
    for (auto it = s_begin; it != s_end; ++it, ++loc.column, ++d_begin, ++i) {
#ifdef EMSCRIPTEN
        using std::begin;
        using std::end;
        auto m_it = std::find_if(begin(map), end(map), [&](auto k) {
#else
        auto m_it = std::ranges::find_if(map, [&](auto k) {
#endif
                return *it == k.front();
        });
        if (m_it == map.end()) {
            throw parse_exception{"Invalid instruction in program: " +
                                      std::to_string(static_cast<int>(*it)),
                                  loc};
        }

        const auto offset = static_cast<int>(i % cipher::size);
        const auto sub = static_cast<int>((*m_it).back()) - offset;

        if (sub < graphical_ascii_range.first) {
            *d_begin = static_cast<char>(sub + cipher::size);
        } else {
            *d_begin = static_cast<char>(sub);
        }
    }

    return d_begin;
}

/** Range-based overload.
 *
 * @tparam InputRange Input range type
 * @tparam OutputIt Output iterator type
 * @param source Source input range
 * @param d_begin Destination begin iterator
 * @return One-past-the-end destination iterator
 * @exception parse_exception Thrown if the program contains errors
 */
template <typename InputRange, typename OutputIt>
OutputIt denormalise_source(InputRange&& source, OutputIt d_begin)
{
    using std::begin;
    using std::end;

    return denormalise_source(begin(source), end(source), std::move(d_begin));
}

/** std::string overload.
 *
 * @param source Program source
 * @return Normalised version of @a source
 * @exception parse_exception Thrown if the program contains errors
 */
std::string denormalise_source(const std::string& source);
}
