/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/algorithm/remove_from_range.hpp"
#include "test_helpers.hpp"

using namespace malbolge;

namespace
{
using tdata = std::tuple<std::vector<int>, std::size_t, std::vector<int>>;
}

BOOST_AUTO_TEST_SUITE(algorithm_suite)

BOOST_AUTO_TEST_CASE(remove_from_range_test)
{
    auto f = [](auto range, auto offset, auto new_range) {
        auto orig_last = std::end(range);
        auto it = std::begin(range) + offset;
        auto last = remove_from_range(it, orig_last);

        BOOST_CHECK(orig_last-1 == last);
        BOOST_REQUIRE_EQUAL((last - std::begin(range)), new_range.size());
        for (auto i = 0u; i < new_range.size(); ++i) {
            BOOST_CHECK_EQUAL(range[i], new_range[i]);
        }
    };

    test::data_set(
        f,
        {
            tdata{{0, 1, 2, 3, 4, 5}, 2, {0, 1, 3, 4, 5}},
            tdata{{0, 1, 2, 3, 4, 5}, 0, {1, 2, 3, 4, 5}},
            tdata{{0, 1, 2, 3, 4, 5}, 5, {0, 1, 2, 3, 4}},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
