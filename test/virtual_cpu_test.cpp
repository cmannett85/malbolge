/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

#include <thread>

using namespace malbolge;
using namespace std::string_literals;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(virtual_cpu_suite)

BOOST_AUTO_TEST_CASE(hello_world)
{
    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto fut = vcpu.run(std::cin, ostr);
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
    auto fut = vcpu.run(istr, ostr);
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

    vcpu.stop();
    fut.get();
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::STOPPED);
}

BOOST_AUTO_TEST_SUITE_END()