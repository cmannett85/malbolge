/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/stream_lock_guard.hpp"

#include "test_helpers.hpp"

#include <mutex>

using namespace malbolge;

BOOST_AUTO_TEST_SUITE(stream_lock_guard_suite)

BOOST_AUTO_TEST_CASE(stream_test)
{
    using tdata = std::tuple<std::ios&, bool>;

    auto mtx = std::mutex{};
    auto f = [&](std::ios& stream, bool expected) {
        auto guard = stream_lock_guard{mtx, stream};

        // Should NOT be locked
        const auto locked = mtx.try_lock();
        BOOST_CHECK_NE(locked, expected);
        if (locked) {
            mtx.unlock();
        }
    };

    auto ss = std::stringstream{};
    test::data_set(
        f,
        {
            tdata{std::cin,   false},
            tdata{std::cerr,  false},
            tdata{std::cout,  false},
            tdata{std::clog,  false},
            tdata{ss,         true},
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()
