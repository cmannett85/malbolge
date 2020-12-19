/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/client_control.hpp"
#include "malbolge/exception.hpp"

#include "test_helpers.hpp"

#include <bitset>
#include <thread>
#include <deque>
#include <condition_variable>

using namespace malbolge;
using namespace debugger;
using namespace std::chrono_literals;

namespace
{
class mock_vcpu
{
public:
    explicit mock_vcpu() :
        skip_configuration{0},
        stop_{false},
        value_ready_{false}
    {}

    ~mock_vcpu()
    {
        stop_ = true;
    }

    void run()
    {
        thread_ = std::jthread{[this]() {
            running(true);

            while (!stop_) {
                std::this_thread::sleep_for(20ms);

                auto cmd = command_queue_type::value_type{command::NONE, {}};
                {
                    auto lock = std::lock_guard{command_queue_mtx_};
                    if (!command_queue_.empty()) {
                        cmd = std::move(command_queue_.front());
                        command_queue_.pop_front();
                    }
                }

                switch (cmd.first) {
                case command::NONE:
                    break;
                case command::PAUSE:
                    cmd.second();
                    break;
                case command::STEP:
                    cmd.second();
                    break;
                case command::RESUME:
                    cmd.second();
                    break;
                case command::ADDRESS:
                case command::REGISTER:
                    {
                        auto lock = std::lock_guard{command_queue_mtx_};
                        value_ready_ = true;
                    }
                    cv_.notify_one();
                    break;
                default:
                    BOOST_TEST_MESSAGE("Unknown command");
                    break;
                }
            }
            running(false);
        }};
    }

    vcpu_control
    configure_debugger(std::function<void (bool)> r,
                       std::function<bool (math::ternary, vcpu_register::id)> s)
    {
        running = std::move(r);
        step_data = std::move(s);

        auto control = vcpu_control{};
        if (!skip_configuration[0]) {
            control.pause = [this](auto cb) {
                auto lock = std::lock_guard{command_queue_mtx_};
                command_queue_.emplace_back(command::PAUSE, std::move(cb));
            };
        }
        if (!skip_configuration[1]) {
            control.step = [this](auto cb) {
                auto lock = std::lock_guard{command_queue_mtx_};
                command_queue_.emplace_back(command::STEP, std::move(cb));
            };
        }
        if (!skip_configuration[2]) {
            control.resume = [this](auto cb) {
                auto lock = std::lock_guard{command_queue_mtx_};
                command_queue_.emplace_back(command::RESUME, std::move(cb));
            };
        }

        if (!skip_configuration[3]) {
            control.address_value = [this](auto address) {
                BOOST_CHECK_EQUAL(address, expected_address);
                {
                    auto lock = std::lock_guard{command_queue_mtx_};
                    command_queue_.emplace_back(command::ADDRESS,
                                                vcpu_control::callback_type{});
                }
                wait_for_value();
                return address_return;
            };
        }
        if (!skip_configuration[4]) {
            control.register_value = [this](auto reg) {
                BOOST_CHECK_EQUAL(reg, expected_register);
                {
                    auto lock = std::lock_guard{command_queue_mtx_};
                    command_queue_.emplace_back(command::REGISTER,
                                                vcpu_control::callback_type{});
                }
                wait_for_value();
                return register_return;
            };
        }

        return control;
    }

    std::bitset<5> skip_configuration;

    math::ternary expected_address;
    math::ternary address_return;

    vcpu_register::id expected_register;
    vcpu_register::data register_return;

    std::function<void (bool)> running;
    std::function<bool (math::ternary, vcpu_register::id)> step_data;

private:
    enum class command {
        NONE,
        PAUSE,
        STEP,
        RESUME,
        ADDRESS,
        REGISTER
    };

    using command_queue_type = std::deque<
        std::pair<command, vcpu_control::callback_type>
    >;

    void wait_for_value()
    {
        auto lock = std::unique_lock{command_queue_mtx_};
        cv_.wait(lock, [this]() { return value_ready_; });
    }

    std::jthread thread_;
    std::atomic_bool stop_;
    std::mutex command_queue_mtx_;
    command_queue_type command_queue_;
    std::condition_variable cv_;
    bool value_ready_;
};
}

BOOST_AUTO_TEST_SUITE(client_control_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    // Good
    {
        auto vcpu = mock_vcpu{};
        auto cc = client_control{vcpu};
        BOOST_CHECK_EQUAL(cc.state(),
                          client_control::execution_state::NOT_RUNNING);
    }

    // Bad
    {
        for (auto i = 1u; i < math::ipow<std::size_t, 2u, 5u>(); ++i) {
            auto vcpu = mock_vcpu{};
            vcpu.skip_configuration = i;

            try {
                auto cc = client_control{vcpu};
                BOOST_FAIL("Should have thrown");
            } catch (basic_exception& e) {
                BOOST_CHECK(true);
            } catch (...) {
                BOOST_CHECK(false);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(pause)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};
    auto cv = std::condition_variable{};
    auto mtx = std::mutex{};

    // Program not running, so should fail
    try {
        cc.pause();
        BOOST_FAIL("Should have thrown");
    } catch (basic_exception&) {
        BOOST_CHECK(true);
    } catch (...) {
        BOOST_CHECK(false);
    }

    vcpu.run();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);

    auto pause_called = false;
    auto cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            pause_called = true;
        }
        cv.notify_one();
    };

    cc.pause(std::move(cb));
    auto lock = std::unique_lock{mtx};
    cv.wait(lock, [&]() { return pause_called; });

    BOOST_CHECK(pause_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);
}

BOOST_AUTO_TEST_CASE(step)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};
    auto cv = std::condition_variable{};
    auto mtx = std::mutex{};

    // Program not running, so should fail
    try {
        cc.step();
        BOOST_FAIL("Should have thrown");
    } catch (basic_exception&) {
        BOOST_CHECK(true);
    } catch (...) {
        BOOST_CHECK(false);
    }

    vcpu.run();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);

    try {
        cc.step();
        BOOST_FAIL("Should have thrown");
    } catch (basic_exception&) {
        BOOST_CHECK(true);
    } catch (...) {
        BOOST_CHECK(false);
    }

    auto pause_called = false;
    auto pause_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            pause_called = true;
        }
        cv.notify_one();
    };

    cc.pause(std::move(pause_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return pause_called; });
    }

    BOOST_CHECK(pause_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);

    auto step_called = false;
    auto step_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            step_called = true;
        }
        cv.notify_one();
    };

    cc.step(std::move(step_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return step_called;});
    }

    BOOST_CHECK(step_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);
}

BOOST_AUTO_TEST_CASE(resume)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};
    auto cv = std::condition_variable{};
    auto mtx = std::mutex{};

    vcpu.run();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);

    auto pause_called = false;
    auto pause_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            pause_called = true;
        }
        cv.notify_one();
    };

    cc.pause(std::move(pause_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return pause_called; });
    }
    BOOST_CHECK(pause_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);

    auto resume_called = false;
    auto resume_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            resume_called = true;
        }
        cv.notify_one();
    };
    cc.resume(std::move(resume_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return resume_called;});
    }
    BOOST_CHECK(resume_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);
}

BOOST_AUTO_TEST_CASE(address_value)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};
    auto cv = std::condition_variable{};
    auto mtx = std::mutex{};

    // Cannot call when running
    vcpu.run();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);

    try {
        cc.address_value(vcpu.expected_address);
        BOOST_FAIL("Should have thrown");
    } catch (basic_exception&) {
        BOOST_CHECK(true);
    } catch (...) {
        BOOST_CHECK(false);
    }

    auto pause_called = false;
    auto pause_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            pause_called = true;
        }
        cv.notify_one();
    };

    cc.pause(std::move(pause_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return pause_called; });
    }
    BOOST_CHECK(pause_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);

    vcpu.expected_address = 42;
    vcpu.address_return = 96;
    auto result = cc.address_value(vcpu.expected_address);
    BOOST_CHECK_EQUAL(result, vcpu.address_return);
}

BOOST_AUTO_TEST_CASE(register_value)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};
    auto cv = std::condition_variable{};
    auto mtx = std::mutex{};

    // Cannot call when running
    vcpu.run();
    std::this_thread::sleep_for(100ms);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::RUNNING);

    try {
        cc.register_value(vcpu.expected_register);
        BOOST_FAIL("Should have thrown");
    } catch (basic_exception&) {
        BOOST_CHECK(true);
    } catch (...) {
        BOOST_CHECK(false);
    }

    auto pause_called = false;
    auto pause_cb = [&]() {
        {
            auto lock = std::lock_guard{mtx};
            pause_called = true;
        }
        cv.notify_one();
    };

    cc.pause(std::move(pause_cb));
    {
        auto lock = std::unique_lock{mtx};
        cv.wait(lock, [&]() { return pause_called; });
    }
    BOOST_CHECK(pause_called);
    BOOST_CHECK_EQUAL(cc.state(), client_control::execution_state::PAUSED);

    vcpu.expected_register = vcpu_register::A;
    vcpu.register_return = vcpu_register::data{24};
    auto result = cc.register_value(vcpu.expected_register);
    BOOST_CHECK_EQUAL(result, vcpu.register_return);

    vcpu.expected_register = vcpu_register::C;
    vcpu.register_return = vcpu_register::data{24, 48};
    result = cc.register_value(vcpu.expected_register);
    BOOST_CHECK_EQUAL(result, vcpu.register_return);
}

BOOST_AUTO_TEST_CASE(default_breakpoint)
{
    auto bp = client_control::breakpoint{42};
    BOOST_CHECK_EQUAL(bp.address(), 42);
    BOOST_CHECK_EQUAL(bp.ignore_count(), 0);

    BOOST_CHECK(bp(vcpu_register::C));
    BOOST_CHECK(bp(vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(breakpoint_cb)
{
    auto return_value = false;
    auto f = [&](auto address, auto reg) {
        BOOST_CHECK_EQUAL(address, 42);
        BOOST_CHECK_EQUAL(reg, vcpu_register::C);
        return return_value;
    };

    auto bp = client_control::breakpoint{42, std::move(f)};
    BOOST_CHECK_EQUAL(bp.address(), 42);
    BOOST_CHECK_EQUAL(bp.ignore_count(), 0);

    BOOST_CHECK(!bp(vcpu_register::C));
    return_value = true;
    BOOST_CHECK(bp(vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(breakpoint_cb_ic)
{
    auto f = [&](auto address, auto reg) {
        BOOST_CHECK_EQUAL(address, 42);
        BOOST_CHECK_EQUAL(reg, vcpu_register::C);
        return true;
    };

    auto bp = client_control::breakpoint{42, std::move(f), 3};
    BOOST_CHECK_EQUAL(bp.address(), 42);
    BOOST_CHECK_EQUAL(bp.ignore_count(), 3);

    BOOST_CHECK(!bp(vcpu_register::C));
    BOOST_CHECK(!bp(vcpu_register::C));
    BOOST_CHECK(!bp(vcpu_register::C));
    BOOST_CHECK(bp(vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(one_breakpoint_no_cb_no_ic)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};

    // Check that firing the step callback doesn't do anything without a
    // breakpoint
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    // Add a breakpoint
    cc.add_breakpoint({3});
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    // Fire it
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::C));

    // Remove
    cc.remove_breakpoint(3);
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(one_breakpoint_no_cb_ic)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};

    // Add a breakpoint
    cc.add_breakpoint({3, client_control::breakpoint::default_callback, 3});
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    // Fire it
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));
}

BOOST_AUTO_TEST_CASE(one_breakpoint_cb_no_ic)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};

    auto expected_address = math::ternary{42};
    auto expected_reg = vcpu_register::C;
    auto f = [&](auto address, auto reg) {
        BOOST_CHECK_EQUAL(address, expected_address);
        BOOST_CHECK_EQUAL(reg, expected_reg);
        return address == 3;
    };

    // Add a breakpoint
    cc.add_breakpoint({3, std::move(f)});
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::C));

    // Fire it
    expected_address = 3;
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::C));
    expected_reg = vcpu_register::D;
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));

    // Remove
    cc.remove_breakpoint(3);
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(one_breakpoint_cb_ic)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};

    auto expected_address = math::ternary{42};
    auto expected_reg = vcpu_register::C;
    auto f = [&](auto address, auto reg) {
        BOOST_CHECK_EQUAL(address, expected_address);
        BOOST_CHECK_EQUAL(reg, expected_reg);
        return address == 3;
    };

    // Add a breakpoint
    cc.add_breakpoint({3, std::move(f), 3});
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::C));

    // Fire it
    expected_address = 3;
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::C));
    expected_reg = vcpu_register::D;
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));

    // Remove
    cc.remove_breakpoint(3);
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::C));
}

BOOST_AUTO_TEST_CASE(three_breakpoints)
{
    auto vcpu = mock_vcpu{};
    auto cc = client_control{vcpu};

    const auto addresses = std::array<math::ternary, 3>{3, 6, 9};
    auto fired = std::array<bool, 3>{false, false, false};
    auto fs = std::array<client_control::breakpoint::callback_type, 3>{
        [&](auto address, auto reg) {
            BOOST_CHECK_EQUAL(address, addresses[0]);
            BOOST_CHECK_EQUAL(reg, vcpu_register::D);
            fired[0] = true;
            return true;
        },
        [&](auto address, auto reg) {
            BOOST_CHECK_EQUAL(address, addresses[1]);
            BOOST_CHECK_EQUAL(reg, vcpu_register::D);
            fired[1] = true;
            return true;
        },
        [&](auto address, auto reg) {
            BOOST_CHECK_EQUAL(address, addresses[2]);
            BOOST_CHECK_EQUAL(reg, vcpu_register::D);
            fired[2] = true;
            return true;
        }
    };
    static_assert(addresses.size() == 3u &&
                  fired.size() == 3u &&
                  fs.size() == 3u,
                  "Test containers must be same size");

    for (auto i = 0u; i < addresses.size(); ++i) {
        cc.add_breakpoint({addresses[i], fs[i]});
        BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));
    }

    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(6, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(9, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    cc.remove_breakpoint(6);
    BOOST_CHECK(vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(6, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(9, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    cc.remove_breakpoint(3);
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(6, vcpu_register::D));
    BOOST_CHECK(vcpu.step_data(9, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));

    cc.remove_breakpoint(9);
    BOOST_CHECK(!vcpu.step_data(3, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(6, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(9, vcpu_register::D));
    BOOST_CHECK(!vcpu.step_data(42, vcpu_register::D));
}

BOOST_AUTO_TEST_SUITE_END()
