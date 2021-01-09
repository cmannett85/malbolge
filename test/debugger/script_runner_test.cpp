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

namespace
{
using func_arg_u = script::function_argument<script::type::uint,
                                         MAL_STR(func_arg_u),
                                         traits::integral_constant<42>>;
using func_arg_t = script::function_argument<script::type::ternary,
                                         MAL_STR(func_arg_t),
                                         traits::integral_constant<42>>;
using func_arg_r = script::function_argument<script::type::reg,
                                         MAL_STR(func_arg_r),
                                         traits::integral_constant<virtual_cpu::vcpu_register::C>>;
using func_arg_s = script::function_argument<script::type::string,
                                         MAL_STR(func_arg_s),
                                         MAL_STR(hello)>;

using func = script::function<
    MAL_STR(test_func),
    script::function_argument<script::type::uint,
                              MAL_STR(arg_a)>,
    script::function_argument<script::type::ternary,
                              MAL_STR(arg_b)>,
    script::function_argument<script::type::reg,
                              MAL_STR(arg_c),
                              traits::integral_constant<virtual_cpu::vcpu_register::C>>,
    script::function_argument<script::type::string,
                              MAL_STR(arg_d),
                              MAL_STR(hello)>
>;
}

BOOST_AUTO_TEST_SUITE(script_runner_suite)

BOOST_AUTO_TEST_SUITE(function_argument_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    static_assert(func_arg_u::name() == "func_arg_u",
                  "function_argument does not match expected");
    static_assert(func_arg_t::name() == "func_arg_t",
                  "function_argument does not match expected");
    static_assert(func_arg_r::name() == "func_arg_r",
                  "function_argument does not match expected");
    static_assert(func_arg_s::name() == "func_arg_s",
                  "function_argument does not match expected");

    BOOST_CHECK_EQUAL(func_arg_u{}.value, 42);
    BOOST_CHECK_EQUAL(func_arg_u{16}.value, 16);
    BOOST_CHECK_EQUAL(func_arg_t{}.value, math::ternary{42});
    BOOST_CHECK_EQUAL(func_arg_t{math::ternary{16}}.value, math::ternary{16});
    BOOST_CHECK_EQUAL(func_arg_t{16}.value, math::ternary{16});
    BOOST_CHECK_EQUAL(func_arg_r{}.value, virtual_cpu::vcpu_register::C);
    BOOST_CHECK_EQUAL(func_arg_r{virtual_cpu::vcpu_register::D}.value,
                      virtual_cpu::vcpu_register::D);
    BOOST_CHECK_EQUAL(func_arg_s{}.value, "hello");
    BOOST_CHECK_EQUAL(func_arg_s{"goodbye"}.value, "goodbye");
}

BOOST_AUTO_TEST_CASE(streaming_operator)
{
    {
        auto ss = std::stringstream{};
        ss << func_arg_u{16};
        BOOST_CHECK_EQUAL(ss.str(), "func_arg_u=16");
    }
    {
        auto ss = std::stringstream{};
        ss << func_arg_t{16};
        BOOST_CHECK_EQUAL(ss.str(), "func_arg_t={d:16, t:0000000121}");
    }
    {
        auto ss = std::stringstream{};
        ss << func_arg_r{virtual_cpu::vcpu_register::A};
        BOOST_CHECK_EQUAL(ss.str(), "func_arg_r=A");
    }
    {
        auto ss = std::stringstream{};
        ss << func_arg_s{"hellooooo"};
        BOOST_CHECK_EQUAL(ss.str(), "func_arg_s=hellooooo");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(function_suite)

BOOST_AUTO_TEST_CASE(constructor)
{
    static_assert(func::name() == "test_func",
                  "function does not match expected");

    // Default construction
    {
        const auto f = func{};
        const auto a = f.value<MAL_STR(arg_a)>();
        const auto b = f.value<MAL_STR(arg_b)>();
        const auto c = f.value<MAL_STR(arg_c)>();
        const auto d = f.value<MAL_STR(arg_d)>();
        BOOST_CHECK_EQUAL(a, 0);
        BOOST_CHECK_EQUAL(b, math::ternary{});
        BOOST_CHECK_EQUAL(c, virtual_cpu::vcpu_register::C);
        BOOST_CHECK_EQUAL(d, "hello");
    }

    // Arbitrary function_argument construction
    {
        const auto f = func{std::tuple_element_t<3, func::args_type>{"goodbye"},
                            std::tuple_element_t<2, func::args_type>{virtual_cpu::vcpu_register::D},
                            std::tuple_element_t<1, func::args_type>{5},
                            std::tuple_element_t<0, func::args_type>{42}};
        const auto a = f.value<MAL_STR(arg_a)>();
        const auto b = f.value<MAL_STR(arg_b)>();
        const auto c = f.value<MAL_STR(arg_c)>();
        const auto d = f.value<MAL_STR(arg_d)>();
        BOOST_CHECK_EQUAL(a, 42);
        BOOST_CHECK_EQUAL(b, math::ternary{5});
        BOOST_CHECK_EQUAL(c, virtual_cpu::vcpu_register::D);
        BOOST_CHECK_EQUAL(d, "goodbye");
    }

    // Forwarding function_argument construction
    {
        const auto f = func{42, 5, virtual_cpu::vcpu_register::D, "goodbye"};
        const auto a = f.value<MAL_STR(arg_a)>();
        const auto b = f.value<MAL_STR(arg_b)>();
        const auto c = f.value<MAL_STR(arg_c)>();
        const auto d = f.value<MAL_STR(arg_d)>();
        BOOST_CHECK_EQUAL(a, 42);
        BOOST_CHECK_EQUAL(b, math::ternary{5});
        BOOST_CHECK_EQUAL(c, virtual_cpu::vcpu_register::D);
        BOOST_CHECK_EQUAL(d, "goodbye");
    }
}

BOOST_AUTO_TEST_CASE(streaming_operator)
{
    auto ss = std::stringstream{};
    ss << func{42, 5, virtual_cpu::vcpu_register::D, "goodbye"};
    BOOST_CHECK_EQUAL(ss.str(),
                      "test_func(arg_a=42, arg_b={d:5, t:0000000012}, arg_c=D, arg_d=goodbye);");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(variant_streaming_operator)
{
    using vdata = std::tuple<script::functions::function_variant, std::string_view>;

    auto f = [](auto var, auto expected) {
        auto ss = std::stringstream{};
        ss << var;
        BOOST_CHECK_EQUAL(ss.str(), expected);
    };

    test::data_set(
        f,
        {
            vdata{script::functions::resume{},
                  "resume();"},
            vdata{script::functions::add_breakpoint{42, 2},
                  "add_breakpoint(address={d:42, t:0000001120}, ignore_count=2);"},
            vdata{script::functions::register_value{virtual_cpu::vcpu_register::D},
                  "register_value(reg=D);"},
        }
    );
}

BOOST_AUTO_TEST_CASE(seq_streaming_operator)
{
    const auto fn_seq = script::functions::sequence{
        script::functions::add_breakpoint{9, 4},
        script::functions::run{},
        script::functions::address_value{6},
        script::functions::register_value{script::type::reg::A},
        script::functions::step{},
        script::functions::resume{},
        script::functions::remove_breakpoint{20}
    };

    constexpr auto expected =
        "add_breakpoint(address={d:9, t:0000000100}, ignore_count=4);\n"
        "run(max_runtime_ms=0);\n"
        "address_value(address={d:6, t:0000000020});\n"
        "register_value(reg=A);\n"
        "step();\n"
        "resume();\n"
        "remove_breakpoint(address={d:20, t:0000000202});\n";

    auto ss = std::stringstream{};
    ss << fn_seq;
    BOOST_CHECK_EQUAL(ss.str(), expected);
}

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
