/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <tuple>
#include <functional>
#include <variant>

namespace malbolge
{
namespace utility
{
/** Generic overloadable visitor, specifically for std::visit.
 *
 * @code
 * const auto offset = 4.2;
 * std::visit(
 *     utility::visitors{
 *         [](auto arg) { std::cout << arg << ' '; },
 *         [&](double arg) { std::cout << (arg + offset) << ' '; },
 *         [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; }
 *     },
 *     v
 * );
 * @endcode
 * @tparam Handlers Callable handler type for each variant type
 */
template<typename... Handlers>
struct visitors : Handlers... {
    /** Constructor
     *
     * @param h Handler instances
     */
    explicit visitors(Handlers&&... h) :
        Handlers{std::forward<Handlers>(h)}...
    {}

    using Handlers::operator()...;
};

/** A wrapper function for std::visit and utility::visitors.
 *
 * The example given for utility::visitors, would become:
 * @code
 * const auto offset = 4.2;
 * utility::visit(
 *     v
 *     [](auto arg) { std::cout << arg << ' '; },
 *     [&](double arg) { std::cout << (arg + offset) << ' '; },
 *     [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; }
 * );
 * @endcode
 * @tparam Var Variant type
 * @tparam Handlers Callable handler type for each variant type
 * @param v Variant instance
 * @param h Handler intstances, there must be at least one provided
 * @return Return of selected handler, follows the same rules as std::visit
 */
template<typename Var, typename... Handlers>
decltype(auto) visit(const Var& v, Handlers&&... h)
{
    static_assert(sizeof...(Handlers) > 0, "Must be at least one handler");
    return std::visit(
        utility::visitors{std::forward<Handlers>(h)...},
        v
    );
}
}
}
