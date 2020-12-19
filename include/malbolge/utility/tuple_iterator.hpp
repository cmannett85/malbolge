/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include <malbolge/traits.hpp>

#include <boost/mp11/algorithm.hpp>

namespace malbolge
{
namespace utility
{
/** Iterates over @a tuple and calls @a f on each element.
 *
 * @code
 * auto t = std::tuple{"hello"s, 42.5, 9};
 * tuple_iterator([](auto i, auto&& v) {
 *     std::cout << i << ": " << v << std::endl;
 * }, t);
 * // 0: hello
 * // 1: 42.5
 * // 2: 9
 * @endcode
 *
 * The signature of @a f must be equivalent to:
 * @code
 * template <typename T>
 * void f(std::size_t, T&&);
 * @endcode
 * @tparam F Callable type
 * @tparam Tuple Tuple type
 * @param f Callable instance
 * @param tuple Tuple instance
 * @return Void
 */
template <typename F, typename Tuple>
constexpr auto tuple_iterator(F&& f, Tuple&& tuple)
    -> std::enable_if_t<traits::is_tuple_like_v<std::decay_t<Tuple>>>
{
    constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
    boost::mp11::mp_for_each<boost::mp11::mp_iota_c<N>>([&](auto i) {
        f(i, std::get<i>(tuple));
    });
}

/** Iterates over @a pack and calls @a f on each instance within it.
 *
 * Forwards @a pack into a tuple and uses it to call
 * tuple_iterator(F&&, Tuple&&).
 * @tparam F Callable type
 * @tparam T Pack types
 * @param f Callable instance
 * @param pack Instances to iterate over
 * @return
 */
template <typename F, typename... T>
constexpr void tuple_iterator(F&& f, T&&... pack)
{
    tuple_iterator(std::forward<F>(f), std::tuple{std::forward<T>(pack)...});
}

/** Iterates over @a tuple and passes a nullptr pointer of the tuple element to
 * @a f.
 *
 * We pass a pointer to @a f because C++20 template lambda support isn't
 * available in the Clang version used by Emscripten.
 * @code
 * Using Tuple = std::tuple<std::string, double, int>;
 * tuple_iterator<Tuple>([](auto i, auto ptr) {
 *     using Arg = std::remove_pointer_t<decltype(ptr)>; 
 *     std::cout << i << ": " << boost::core::demangle(typeid(Arg).name())
 *               << std::endl;
 * }, t);
 * // 0: std::string
 * // 1: double
 * // 2: int
 * @endcode
 *
 * The signature of @a f must be equivalent to:
 * @code
 * template <typename T>
 * void f(std::size_t is, T* ptr);
 * @endcode
 * @tparam Tuple Tuple type
 * @tparam F Callable type
 * @param f Callable instance
 * @return Void
 */
template <typename Tuple, typename F>
constexpr auto tuple_type_iterator(F&& f)
    -> std::enable_if_t<traits::is_tuple_like_v<std::decay_t<Tuple>>>
{
    constexpr auto N = std::tuple_size_v<std::decay_t<Tuple>>;
    boost::mp11::mp_for_each<boost::mp11::mp_iota_c<N>>([&](auto i) {
        f(i, std::add_pointer_t<std::tuple_element_t<i, Tuple>>{nullptr});
    });
}
}
}
