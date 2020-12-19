/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_runner.hpp"
#include "malbolge/virtual_cpu.hpp"
#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace debugger;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(script_runner_suite)

BOOST_AUTO_TEST_CASE(hello_world_debugger)
{
    constexpr auto dstr_expected =
R"([DBGR]: address_value(address={d:9, t:0000000100}) = {d:125, t:0000011122}
[DBGR]: register_value(reg=A) = {{}, {d:72, t:0000002200}}
[DBGR]: register_value(reg=C) = {{d:9, t:0000000100}, {d:125, t:0000011122}}
[DBGR]: register_value(reg=D) = {{d:62, t:0000002022}, {d:37, t:0000001101}}
[DBGR]: address_value(address={d:10, t:0000000101}) = {d:124, t:0000011121}
[DBGR]: register_value(reg=A) = {{}, {d:72, t:0000002200}}
[DBGR]: register_value(reg=C) = {{d:10, t:0000000101}, {d:124, t:0000011121}}
[DBGR]: register_value(reg=D) = {{d:38, t:0000001102}, {d:61, t:0000002021}}
[DBGR]: address_value(address={d:19, t:0000000201}) = {d:80, t:0000002222}
[DBGR]: register_value(reg=A) = {{}, {d:9836, t:0111111022}}
[DBGR]: register_value(reg=C) = {{d:19, t:0000000201}, {d:80, t:0000002222}}
[DBGR]: register_value(reg=D) = {{d:37, t:0000001101}, {d:125, t:0000011122}}
)";

    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto dstr = std::stringstream{};

    auto fn_seq = script::functions::sequence{
        script::functions::add_breakpoint{9},
        script::functions::add_breakpoint{19},
        script::functions::run{},
        script::functions::address_value{9},
        script::functions::register_value{script::type::reg::A},
        script::functions::register_value{script::type::reg::C},
        script::functions::register_value{script::type::reg::D},

        script::functions::step{},
        script::functions::address_value{10},
        script::functions::register_value{script::type::reg::A},
        script::functions::register_value{script::type::reg::C},
        script::functions::register_value{script::type::reg::D},

        script::functions::resume{},
        script::functions::address_value{19},
        script::functions::register_value{script::type::reg::A},
        script::functions::register_value{script::type::reg::C},
        script::functions::register_value{script::type::reg::D},

        script::functions::add_breakpoint{20},
        script::functions::remove_breakpoint{20},
        script::functions::resume{},
        script::functions::address_value{20},
    };

    script::run(fn_seq, vcpu, dstr, ostr);

    BOOST_CHECK_EQUAL(ostr.str(), "Hello World!");

    // We have to strip the timestamp off each dstr line
    auto result = ""s;
    for (auto line = ""s; std::getline(dstr, line); ) {
        result += line.substr(34, line.size()-38) + '\n';
    }

    BOOST_CHECK_EQUAL(result, dstr_expected);
}

BOOST_AUTO_TEST_CASE(echo_debugger)
{
    constexpr auto ostr_expected = "Hello!\nGoodbye!\n";
    constexpr auto dstr_expected = "";

    auto vmem = load(std::filesystem::path{"programs/echo.mal"});
    auto vcpu = virtual_cpu{std::move(vmem)};
    BOOST_CHECK_EQUAL(vcpu.state(), virtual_cpu::execution_state::READY);

    auto ostr = std::stringstream{};
    auto dstr = std::stringstream{};

    auto fn_seq = script::functions::sequence{
        script::functions::on_input{"Hello!"},
        script::functions::on_input{"Goodbye!"},
        script::functions::run{100},
    };

    script::run(fn_seq, vcpu, dstr, ostr);

    BOOST_CHECK_EQUAL(ostr.str(), ostr_expected);
    BOOST_CHECK_EQUAL(dstr.str(), dstr_expected);
}

BOOST_AUTO_TEST_SUITE_END()
