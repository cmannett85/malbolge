/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/tuple_iterator.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(tuple_iterator_suite)

BOOST_AUTO_TEST_CASE(tuple_iterator_test)
{
    auto ss = std::stringstream{};
    auto t = std::tuple{"hello"s, 42.5, 9};

    utility::tuple_iterator([&](auto i, auto&& v) {
        ss << i << ": " << v;
        if constexpr (i < (std::tuple_size_v<std::decay_t<decltype(t)>>-1)) {
            ss << ", ";
        }
    }, t);

    BOOST_CHECK_EQUAL(ss.str(), "0: hello, 1: 42.5, 2: 9");
}

BOOST_AUTO_TEST_CASE(pack_tuple_iterator_test)
{
    auto ss = std::stringstream{};

    utility::tuple_iterator([&](auto i, auto&& v) {
        ss << i << ": " << v;
        if constexpr (i < 2) {
            ss << ", ";
        }
    }, "hello"s, 42.5, 9);

    BOOST_CHECK_EQUAL(ss.str(), "0: hello, 1: 42.5, 2: 9");
}

BOOST_AUTO_TEST_CASE(tuple_type_iterator_test)
{
    using Tuple = std::tuple<std::string, double, int>;

    utility::tuple_type_iterator<Tuple>([](auto i, auto ptr) {
        using T = std::remove_pointer_t<decltype(ptr)>;

        const auto result = std::is_same_v<std::tuple_element_t<i, Tuple>, T>;
        BOOST_CHECK(result);
    });
}

BOOST_AUTO_TEST_SUITE_END()
