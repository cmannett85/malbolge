/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

#include <thread>

using namespace malbolge;
using namespace debugger;
using namespace std::string_literals;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(virtual_cpu_suite)

BOOST_AUTO_TEST_CASE(hello_world)
{
    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto mtx = std::mutex{};
    auto fut = vcpu.run(std::cin, ostr, mtx);
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::RUNNING);

    fut.get();
    BOOST_CHECK_EQUAL(ostr.str(), "Hello World!");
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
}

BOOST_AUTO_TEST_CASE(hello_world_string)
{
    auto vmem = load(R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>)"s);
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto mtx = std::mutex{};
    auto fut = vcpu.run(std::cin, ostr, mtx);
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::RUNNING);

    fut.get();
    BOOST_CHECK_EQUAL(ostr.str(), "Hello World!");
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
}

BOOST_AUTO_TEST_CASE(hello_world_string_normalised)
{
    auto vmem = load("jjjjpp<jjjj*p<jjjpp<<jjjj*p<jj*o*<i<io<</<<oo<*o*<jvoo<<opj<*<<<<<ojjopjp<jio<ovo<<jo<p*o<*jo<iooooo<jj*p<jji<oo<j*jp<jj**p<jjopp<i"s, true);
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto mtx = std::mutex{};
    auto fut = vcpu.run(std::cin, ostr, mtx);
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::RUNNING);

    fut.get();
    BOOST_CHECK_EQUAL(ostr.str(), "Hello World!");
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
}

BOOST_AUTO_TEST_CASE(echo)
{
    auto vmem = load(std::filesystem::path{"programs/echo.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto istr = std::stringstream{};
    auto ostr = std::stringstream{};
    auto mtx = std::mutex{};
    auto fut = vcpu.run(istr, ostr, mtx);
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::RUNNING);

    auto f = [&](auto input) {
        auto buf = ""s;

        istr.clear();
        istr.str("");
        ostr.clear();
        ostr.str("");

        istr << input << std::endl;
        std::this_thread::sleep_for(100ms);
        ostr >> buf;

        BOOST_CHECK_EQUAL(buf, input);
    };

    test::data_set(
        f,
        {
            std::tuple{"Hello"s},
            std::tuple{"Test!"s},
            std::tuple{"Goodbye..."s},
        }
    );

    // Wait a moment so we are back into waiting for input (to test stopping)
    // Issue #97
    std::this_thread::sleep_for(100ms);

    vcpu.stop();
    fut.get();
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
}

BOOST_AUTO_TEST_CASE(hello_world_debugger)
{
    auto dbg_cv = std::condition_variable{};
    auto dbg_mtx = std::mutex{};

    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    auto dbg = client_control{vcpu};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);
    BOOST_CHECK_EQUAL(dbg.state(), client_control::execution_state::NOT_RUNNING);

    auto expected_address = math::ternary{9};
    auto callback_reached = false;
    auto bp_cb = [&](auto address, auto reg) {
        BOOST_CHECK_EQUAL(address, expected_address);
        BOOST_CHECK_EQUAL(reg, vcpu_register::C);

        {
            auto lock = std::lock_guard{dbg_mtx};
            callback_reached = true;
        }
        dbg_cv.notify_one();

        return true;
    };
    dbg.add_breakpoint({expected_address, bp_cb});
    dbg.add_breakpoint({expected_address + 10, std::move(bp_cb)});

    auto ostr = std::stringstream{};
    auto mtx = std::mutex{};
    auto fut = vcpu.run(std::cin, ostr, mtx);
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::RUNNING);

    {
        // BP1
        {
            auto lock = std::unique_lock{dbg_mtx};
            dbg_cv.wait(lock, [&]() { return callback_reached; });
        }
        BOOST_CHECK_EQUAL(dbg.state(), client_control::execution_state::PAUSED);
        BOOST_CHECK_EQUAL(dbg.address_value(expected_address),
                          math::ternary{125});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::A),
                          vcpu_register::data{72});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::C),
                          (vcpu_register::data{9, 125}));
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::D),
                          (vcpu_register::data{62, 37}));

        BOOST_TEST_MESSAGE("Before step");
        dbg.step();
        std::this_thread::sleep_for(100ms);
        BOOST_CHECK_EQUAL(dbg.state(), client_control::execution_state::PAUSED);
        BOOST_CHECK_EQUAL(dbg.address_value(expected_address+1),
                          math::ternary{124});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::A),
                          vcpu_register::data{72});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::C),
                          (vcpu_register::data{10, 124}));
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::D),
                          (vcpu_register::data{38, 61}));

        BOOST_TEST_MESSAGE("After step");

        // BP2
        callback_reached = false;
        expected_address += 10;
        dbg.resume();
        {
            auto lock = std::unique_lock{dbg_mtx};
            dbg_cv.wait(lock, [&]() { return callback_reached; });
        }
        BOOST_CHECK_EQUAL(dbg.state(), client_control::execution_state::PAUSED);
        BOOST_CHECK_EQUAL(dbg.address_value(expected_address),
                          math::ternary{80});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::A),
                          vcpu_register::data{9836});
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::C),
                          (vcpu_register::data{19, 80}));
        BOOST_CHECK_EQUAL(dbg.register_value(vcpu_register::D),
                          (vcpu_register::data{37, 125}));

        dbg.resume();
    }

    fut.get();
    BOOST_CHECK_EQUAL(ostr.str(), "Hello World!");
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
    BOOST_CHECK_EQUAL(dbg.state(), client_control::execution_state::NOT_RUNNING);
    BOOST_CHECK(callback_reached);
}

BOOST_AUTO_TEST_SUITE_END()
