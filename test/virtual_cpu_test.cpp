/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

#include <bitset>
#include <condition_variable>

using namespace malbolge;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace
{
template <std::size_t N>
void fire_bit(std::mutex& mtx,
              std::condition_variable& cv,
              std::bitset<N>& bitset,
              std::size_t idx)
{
    {
        auto lk = std::lock_guard{mtx};
        bitset[idx] = true;
    }
    cv.notify_one();
}

void check_state(virtual_cpu::execution_state state,
                 virtual_cpu::execution_state test_state,
                 std::mutex& mtx,
                 std::condition_variable& cv,
                 bool& flag)
{
    if (state == test_state) {
        {
            auto lk = std::lock_guard{mtx};
            flag = true;
        }
        cv.notify_one();
    }
}
}

BOOST_AUTO_TEST_SUITE(virtual_cpu_suite)

BOOST_AUTO_TEST_CASE(hello_world)
{
    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto stopped = false;

    vcpu.register_for_breakpoint_hit_signal([&](auto) {
        BOOST_CHECK_MESSAGE(false, "Unexpected breakpoint signal");
    });

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::STOPPED,
    };
    auto state_count = 0u;
    vcpu.register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;
        check_state(state, expected_states.back(), mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu.run();
    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK_EQUAL(state_count, 2u);
}

BOOST_AUTO_TEST_CASE(hello_world_string)
{
    auto vmem = load(R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>)"s);
    auto vcpu = virtual_cpu{std::move(vmem)};
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto stopped = false;

    vcpu.register_for_breakpoint_hit_signal([&](auto) {
        BOOST_CHECK_MESSAGE(false, "Unexpected breakpoint signal");
    });

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::STOPPED,
    };
    auto state_count = 0u;
    vcpu.register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;
        check_state(state, expected_states.back(), mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu.run();
    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK_EQUAL(state_count, 2u);
}

BOOST_AUTO_TEST_CASE(hello_world_string_normalised)
{
    auto vmem = load("jjjjpp<jjjj*p<jjjpp<<jjjj*p<jj*o*<i<io<</<<oo<*o*<jvoo<<opj<*<<<<<ojjopjp<jio<ovo<<jo<p*o<*jo<iooooo<jj*p<jji<oo<j*jp<jj**p<jjopp<i"s);
    auto vcpu = virtual_cpu{std::move(vmem)};
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto stopped = false;

    vcpu.register_for_breakpoint_hit_signal([&](auto) {
        BOOST_CHECK_MESSAGE(false, "Unexpected breakpoint signal");
    });

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::STOPPED,
    };
    auto state_count = 0u;
    vcpu.register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;
        check_state(state, expected_states.back(), mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu.run();
    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK_EQUAL(state_count, 2u);
}

BOOST_AUTO_TEST_CASE(echo_preload)
{
    auto vmem = load(std::filesystem::path{"programs/echo.mal"});
    auto vcpu = std::make_unique<virtual_cpu>(std::move(vmem));
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto waiting = false;
    auto stopped = false;

    vcpu->register_for_breakpoint_hit_signal([&](auto) {
        BOOST_CHECK_MESSAGE(false, "Unexpected breakpoint signal");
    });

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::WAITING_FOR_INPUT,
        virtual_cpu::execution_state::STOPPED
    };
    auto state_count = 0u;
    vcpu->register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;

        check_state(state, expected_states[1], mtx, cv, waiting);
        check_state(state, expected_states[2], mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu->register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu->add_input("Hello\n");
    vcpu->add_input("Test!\n");
    vcpu->add_input("Goodbye...\n");

    vcpu->run();
    {
        auto lk = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return waiting; }));
    }

    vcpu.reset();
    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "Hello\nTest!\nGoodbye...\n");
    BOOST_CHECK_EQUAL(state_count, 3u);
}

BOOST_AUTO_TEST_CASE(hello_world_debugger)
{
    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto stopped = false;
    auto paused = false;

    auto expected_address = math::ternary{9};
    auto bp_reached = false;
    vcpu.register_for_breakpoint_hit_signal([&](auto address) {
        BOOST_CHECK_EQUAL(address, expected_address);
        {
            auto lock = std::lock_guard{mtx};
            bp_reached = true;
        }
        cv.notify_one();
    });
    vcpu.add_breakpoint(expected_address);
    vcpu.add_breakpoint(expected_address + 10);
    vcpu.add_breakpoint(expected_address + 11);

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::PAUSED,     // BP1, Step
        virtual_cpu::execution_state::RUNNING,    // Resume
        virtual_cpu::execution_state::PAUSED,     // BP2
        virtual_cpu::execution_state::RUNNING,    // Resume
        virtual_cpu::execution_state::PAUSED,     // Pause
        virtual_cpu::execution_state::RUNNING,    // Resume
        virtual_cpu::execution_state::STOPPED,    // Stopped
    };
    auto state_count = 0u;
    vcpu.register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;

        check_state(state, virtual_cpu::execution_state::PAUSED, mtx, cv, paused);
        check_state(state, virtual_cpu::execution_state::STOPPED, mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu.run();
    {
        // BP1
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return bp_reached; }));
        }
        bp_reached = false;
        expected_address += 10;
        paused = false;

        auto value_hits = std::bitset<4>{};
        vcpu.address_value(math::ternary{9},
                           [&](auto address, auto value) {
            BOOST_CHECK_EQUAL(address, math::ternary{9});
            BOOST_CHECK_EQUAL(value, math::ternary{125});
            fire_bit(mtx, cv, value_hits, 0);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::A,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::A);
            BOOST_CHECK(!address);
            BOOST_CHECK_EQUAL(value, math::ternary{72});
            fire_bit(mtx, cv, value_hits, 1);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::C,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::C);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{9});
            BOOST_CHECK_EQUAL(value, math::ternary{125});
            fire_bit(mtx, cv, value_hits, 2);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::D,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::D);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{62});
            BOOST_CHECK_EQUAL(value, math::ternary{37});
            fire_bit(mtx, cv, value_hits, 3);
        });
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return value_hits.all(); }));
        }

        // Step
        value_hits.reset();
        vcpu.step();

        vcpu.address_value(math::ternary{10},
                           [&](auto address, auto value) {
            BOOST_CHECK_EQUAL(address, math::ternary{10});
            BOOST_CHECK_EQUAL(value, math::ternary{124});
            fire_bit(mtx, cv, value_hits, 0);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::A,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::A);
            BOOST_CHECK(!address);
            BOOST_CHECK_EQUAL(value, math::ternary{72});
            fire_bit(mtx, cv, value_hits, 1);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::C,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::C);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{10});
            BOOST_CHECK_EQUAL(value, math::ternary{124});
            fire_bit(mtx, cv, value_hits, 2);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::D,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::D);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{38});
            BOOST_CHECK_EQUAL(value, math::ternary{61});
            fire_bit(mtx, cv, value_hits, 3);
        });
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return value_hits.all(); }));
        }

        // Resume
        vcpu.run();

        // BP2
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return bp_reached; }));
        }
        bp_reached = false;
        paused = false;

        value_hits.reset();
        vcpu.address_value(math::ternary{19},
                           [&](auto address, auto value) {
            BOOST_CHECK_EQUAL(address, math::ternary{19});
            BOOST_CHECK_EQUAL(value, math::ternary{80});
            fire_bit(mtx, cv, value_hits, 0);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::A,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::A);
            BOOST_CHECK(!address);
            BOOST_CHECK_EQUAL(value, math::ternary{9836});
            fire_bit(mtx, cv, value_hits, 1);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::C,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::C);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{19});
            BOOST_CHECK_EQUAL(value, math::ternary{80});
            fire_bit(mtx, cv, value_hits, 2);
        });
        vcpu.register_value(virtual_cpu::vcpu_register::D,
                            [&](auto reg, auto address, auto value) {
            BOOST_CHECK_EQUAL(reg, virtual_cpu::vcpu_register::D);
            BOOST_REQUIRE(address);
            BOOST_CHECK_EQUAL(*address, math::ternary{37});
            BOOST_CHECK_EQUAL(value, math::ternary{125});
            fire_bit(mtx, cv, value_hits, 3);
        });
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return value_hits.all(); }));
        }

        vcpu.remove_breakpoint(math::ternary{20});

        // Resume and then immediately pause.  The now removed BP3 should not
        // fire
        vcpu.run();
        vcpu.pause();

        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return paused; }));
        }

        // Resume to finish
        vcpu.run();
    }

    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK_EQUAL(state_count, expected_states.size());
}

BOOST_AUTO_TEST_CASE(debugger_ignore_count)
{
    auto vmem = load(std::filesystem::path{"programs/echo.mal"});
    auto vcpu = std::make_shared<virtual_cpu>(std::move(vmem));
    auto mtx = std::mutex{};
    auto cv = std::condition_variable{};
    auto stopped = false;
    auto waiting = false;

    const auto expected_address = math::ternary{37};
    auto bp_reached = false;
    vcpu->register_for_breakpoint_hit_signal([&](auto address) {
        BOOST_CHECK_EQUAL(address, expected_address);
        {
            auto lock = std::lock_guard{mtx};
            bp_reached = true;
        }
        cv.notify_one();
    });
    vcpu->add_breakpoint(expected_address, 17);

    auto expected_states = std::array{
        virtual_cpu::execution_state::RUNNING,
        virtual_cpu::execution_state::PAUSED,             // BP
        virtual_cpu::execution_state::RUNNING,            // Resume
        virtual_cpu::execution_state::PAUSED,             // BP
        virtual_cpu::execution_state::RUNNING,            // Resume
        virtual_cpu::execution_state::WAITING_FOR_INPUT,
        virtual_cpu::execution_state::STOPPED,            // Stopped
    };
    auto state_count = 0u;
    vcpu->register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (std::exception& e) {
                BOOST_TEST_MESSAGE(e.what());
            }
            BOOST_CHECK_MESSAGE(false, "Unexpected error signal");
        }

        BOOST_CHECK_EQUAL(state, expected_states[state_count]);
        ++state_count;

        check_state(state, virtual_cpu::execution_state::WAITING_FOR_INPUT, mtx, cv, waiting);
        check_state(state, virtual_cpu::execution_state::STOPPED, mtx, cv, stopped);
    });

    auto output_str = ""s;
    vcpu->register_for_output_signal([&](auto c) {
        output_str += c;
    });

    vcpu->add_input("a");

    vcpu->run();
    {
        // BP
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return bp_reached; }));
        }
        bp_reached = false;

        auto value_hits = std::bitset<1>{};
        vcpu->address_value(expected_address,
                           [&](auto address, auto value) {
            BOOST_CHECK_EQUAL(address, expected_address);
            BOOST_CHECK_EQUAL(value, math::ternary{50});
            fire_bit(mtx, cv, value_hits, 0);
        });
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return value_hits.all(); }));
        }

        // Resume
        vcpu->run();

        // BP
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return bp_reached; }));
        }
        bp_reached = false;

        value_hits.reset();
        vcpu->address_value(expected_address,
                           [&](auto address, auto value) {
            BOOST_CHECK_EQUAL(address, expected_address);
            BOOST_CHECK_EQUAL(value, math::ternary{80});
            fire_bit(mtx, cv, value_hits, 0);
        });
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return value_hits.all(); }));
        }

        // Resume
        vcpu->run();
        {
            auto lk = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return waiting; }));
        }

        // Stop
        vcpu.reset();
    }

    auto lk = std::unique_lock{mtx};
    BOOST_CHECK(cv.wait_for(lk, 100ms, [&]() { return stopped; }));

    BOOST_CHECK_EQUAL(output_str, "a");
    BOOST_CHECK_EQUAL(state_count, expected_states.size());
}

BOOST_AUTO_TEST_SUITE_END()
