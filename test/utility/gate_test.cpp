/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/gate.hpp"

#include "test_helpers.hpp"

#include <thread>

using namespace malbolge;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(gate_suite)

BOOST_AUTO_TEST_CASE(gate_destroyed_open)
{
    auto g = utility::gate{};
    g.close();

    auto counter = std::atomic_size_t{0};
    auto thread = std::thread{[&]() {
        for (; counter < 100'000u; ++counter) {
            g();
        }
    }};

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(counter.load(), 0);

    g.open(100);
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(counter.load(), 100);

    g.open(1);
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(counter.load(), 101);

    g.open();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(counter.load(), 100'000);

    thread.join();
}

BOOST_AUTO_TEST_CASE(gate_destroyed_closed)
{
    auto g = std::make_unique<utility::gate>();
    g->close();

    auto counter = std::atomic_size_t{0};
    auto thread = std::thread{[&]() {
        for (; counter < 100'000u; ++counter) {
            (*g)();
            if (!g) {
                break;
            }
        }
    }};

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(counter.load(), 0);

    g = nullptr;
    thread.join();
    BOOST_CHECK_EQUAL(counter.load(), 0);
}

BOOST_AUTO_TEST_CASE(gate_notifier)
{
    auto is_closed = false;
    auto notifier = [&](bool closed) {
        is_closed = closed;
    };

    auto g = utility::gate{};
    g.close();

    auto thread = std::thread{[&]() {
            g(notifier);
    }};

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(is_closed);

    g.open();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(!is_closed);

    thread.join();
}

BOOST_AUTO_TEST_SUITE_END()
