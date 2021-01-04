/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/signal.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(signal_suite)

BOOST_AUTO_TEST_CASE(single_arg_single_slot)
{
    auto result = ""s;
    auto slot = [&](auto a) {
        result = a;
    };

    auto sig = utility::signal<std::string>{};
    sig.connect(slot);

    sig("hello");
    BOOST_CHECK_EQUAL(result, "hello");
}

BOOST_AUTO_TEST_CASE(multiple_arg_single_slot)
{
    auto result = ""s;
    auto slot = [&](auto a, auto b) {
        result = a + std::to_string(b);
    };

    auto sig = utility::signal<std::string, int>{};
    sig.connect(slot);

    sig("hello: ", 3);
    BOOST_CHECK_EQUAL(result, "hello: 3");
}

BOOST_AUTO_TEST_CASE(multiple_arg_multiple_slot)
{
    auto results = std::array<std::string, 3>{};
    auto slots = std::array<std::function<void (std::string, int)>, 3>{};
    static_assert(results.size() == slots.size(),
                  "Results and slots must be same size");

    auto sig = utility::signal<std::string, int>{};
    for (auto i = 0u; i < results.size(); ++i) {
        slots[i] = [&, i](std::string a, int b) {
            results[i] = a + std::to_string(b);
        };
        sig.connect(slots[i]);
    }

    sig("hello: ", 3);
    for (const auto& result : results) {
        BOOST_CHECK_EQUAL(result, "hello: 3");
    }
}

BOOST_AUTO_TEST_CASE(multiple_arg_multiple_slot_copy)
{
    auto results = std::array<std::string, 3>{};
    auto slots = std::array<std::function<void (std::string, int)>, 3>{};
    static_assert(results.size() == slots.size(),
                  "Results and slots must be same size");

    auto sig1 = utility::signal<std::string, int>{};
    for (auto i = 0u; i < results.size(); ++i) {
        slots[i] = [&, i](std::string a, int b) {
            results[i] = a + std::to_string(b);
        };
        sig1.connect(slots[i]);
    }

    auto sig2 = sig1;

    sig1("hello: ", 3);
    for (auto& result : results) {
        BOOST_CHECK_EQUAL(result, "hello: 3");
        result = ""s;
    }

    sig2("goodbye ", 42);
    for (const auto& result : results) {
        BOOST_CHECK_EQUAL(result, "goodbye 42");
    }
}

BOOST_AUTO_TEST_CASE(single_slot_disconnect)
{
    auto result = "hello"s;
    auto slot = [&](auto a) {
        result = a;
    };

    auto sig = utility::signal<std::string>{};
    auto conn = sig.connect(slot);

    conn.disconnect();

    sig("goodbye");
    BOOST_CHECK_EQUAL(result, "hello");
}

BOOST_AUTO_TEST_CASE(single_slot_disconnect_after_move)
{
    auto result = ""s;
    auto slot = [&](auto a) {
        result = a;
    };

    auto sig1 = utility::signal<std::string>{};
    auto conn = sig1.connect(slot);

    auto sig2 = std::move(sig1);
    sig2("hello");
    BOOST_CHECK_EQUAL(result, "hello");

    conn.disconnect();

    sig2("goodbye");
    BOOST_CHECK_EQUAL(result, "hello");
}

BOOST_AUTO_TEST_CASE(single_slot_disconnect_after_destroy)
{
    auto result = ""s;
    auto slot = [&](auto a) {
        result = a;
    };

    auto sig = std::make_shared<utility::signal<std::string>>();
    auto conn = sig->connect(slot);
    sig.reset();

    // The test here is for it to not crash...
    conn.disconnect();
}

BOOST_AUTO_TEST_CASE(multiple_slot_disconnect)
{
    auto results = std::array<std::string, 3>{};
    auto slots = std::array<std::function<void (std::string)>, 3>{};
    static_assert(results.size() == slots.size(),
                  "Results and slots must be same size");

    auto sig = utility::signal<std::string>{};
    auto conns = std::vector<utility::signal<std::string>::connection>{};
    conns.reserve(results.size());

    for (auto i = 0u; i < results.size(); ++i) {
        slots[i] = [&, i](std::string a) {
            results[i] = a;
        };
        conns.push_back(sig.connect(slots[i]));
    }
    BOOST_CHECK_EQUAL(conns.size(), results.size());

    conns[1].disconnect();

    sig("hello");
    BOOST_CHECK_EQUAL(results[0], "hello");
    BOOST_CHECK_EQUAL(results[1], "");
    BOOST_CHECK_EQUAL(results[2], "hello");
}

BOOST_AUTO_TEST_SUITE_END()
