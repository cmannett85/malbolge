/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/raii.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(raii_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    auto fired = false;
    {
        auto raii = utility::raii{[&]() { fired = true; }};
    }
    BOOST_CHECK(fired);
}

BOOST_AUTO_TEST_SUITE_END()
