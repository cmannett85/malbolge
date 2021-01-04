/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_runner.hpp"
#include "malbolge/loader.hpp"

#include "test_helpers.hpp"

#include <deque>

using namespace malbolge;
using namespace debugger;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(script_runner_suite)

BOOST_AUTO_TEST_CASE(hello_world_debugger)
{
    auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
    auto runner = script::script_runner{};

    auto output_str = ""s;
    runner.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    using address_value_args = traits::arg_extractor<script::script_runner::address_value_signal_type>;
    auto address_expected = std::deque<address_value_args>{
        {9, 125},
        {10, 124},
        {19, 80}
    };
    runner.register_for_address_value_signal([&](auto fn, auto value) {
        BOOST_REQUIRE(!address_expected.empty());

        const auto& expected = address_expected.front();
        BOOST_CHECK_EQUAL(std::get<0>(expected), fn);
        BOOST_CHECK_EQUAL(std::get<1>(expected), value);

        address_expected.pop_front();
    });

    using reg_value_args = traits::arg_extractor<script::script_runner::register_value_signal_type>;
    auto reg_expected = std::deque<reg_value_args>{
        {virtual_cpu::vcpu_register::A, {},  72},
        {virtual_cpu::vcpu_register::C,  9, 125},
        {virtual_cpu::vcpu_register::D, 62,  37},

        {virtual_cpu::vcpu_register::A, {},  72},
        {virtual_cpu::vcpu_register::C, 10, 124},
        {virtual_cpu::vcpu_register::D, 38,  61},

        {virtual_cpu::vcpu_register::A, {}, 9836},
        {virtual_cpu::vcpu_register::C, 19,   80},
        {virtual_cpu::vcpu_register::D, 37,  125},
    };
    runner.register_for_register_value_signal([&](auto fn, auto address, auto value) {
        BOOST_REQUIRE(!reg_expected.empty());

        const auto& expected = reg_expected.front();
        BOOST_CHECK_EQUAL(std::get<0>(expected), fn);
        BOOST_CHECK_EQUAL(std::get<1>(expected), address);
        BOOST_CHECK_EQUAL(std::get<2>(expected), value);

        reg_expected.pop_front();
    });

    const auto fn_seq = script::functions::sequence{
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

    runner.run(std::move(vmem), fn_seq);

    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK(address_expected.empty());
    BOOST_CHECK(reg_expected.empty());
}

BOOST_AUTO_TEST_CASE(echo_debugger)
{
    auto vmem = load(std::filesystem::path{"programs/echo.mal"});
    auto runner = script::script_runner{};

    auto output_str = ""s;
    runner.register_for_output_signal([&](auto c) {
        output_str += c;
    });

    runner.register_for_address_value_signal([&](auto, auto) {
        BOOST_CHECK_MESSAGE(false, "Signal should not have fired");
    });
    runner.register_for_register_value_signal([&](auto, auto, auto) {
        BOOST_CHECK_MESSAGE(false, "Signal should not have fired");
    });

    const auto fn_seq = script::functions::sequence{
        script::functions::on_input{"Hello!\n"},
        script::functions::on_input{"Goodbye!\n"},
        script::functions::run{100},
    };

    runner.run(std::move(vmem), fn_seq);

    BOOST_CHECK_EQUAL(output_str, "Hello!\nGoodbye!\n");
}

BOOST_AUTO_TEST_CASE(validation)
{
    auto f = [](auto fn_seq) {
        auto vmem = load(std::filesystem::path{"programs/hello_world.mal"});
        auto runner = script::script_runner{};

        try {
            runner.run(std::move(vmem), fn_seq);
            BOOST_CHECK_MESSAGE(false, "Should have thrown");
        } catch (std::exception& e) {
            BOOST_TEST_MESSAGE(e.what());
        }
    };

    test::data_set(
        f,
        {
            std::tuple{
                script::functions::sequence{}
            },
            std::tuple{
                script::functions::sequence{
                    script::functions::add_breakpoint{9},
                    script::functions::add_breakpoint{19},
                }
            },
            std::tuple{
                script::functions::sequence{
                    script::functions::run{},
                    script::functions::run{100},
                }
            },
            std::tuple{
                script::functions::sequence{
                    script::functions::step{},
                    script::functions::run{100},
                }
            },
            std::tuple{
                script::functions::sequence{
                    script::functions::resume{},
                    script::functions::run{100},
                }
            },
            std::tuple{
                script::functions::sequence{
                    script::functions::run{100},
                    script::functions::add_breakpoint{9},
                }
            },
        }
    );
}

BOOST_AUTO_TEST_CASE(program_error)
{
    auto program = "jjjjpp<jjjj*p<jjjpp<<jjjj*p"s;
    auto vmem = load(program);
    auto runner = script::script_runner{};

    runner.register_for_address_value_signal([&](auto, auto) {
        BOOST_CHECK_MESSAGE(false, "Signal should not have fired");
    });
    runner.register_for_register_value_signal([&](auto, auto, auto) {
        BOOST_CHECK_MESSAGE(false, "Signal should not have fired");
    });

    const auto fn_seq = script::functions::sequence{
        script::functions::run{},
    };

    try {
        runner.run(std::move(vmem), fn_seq);
        BOOST_CHECK_MESSAGE(false, "Should have thrown");
    } catch (std::exception& e) {
        BOOST_TEST_MESSAGE(e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()
