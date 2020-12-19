/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_parser.hpp"
#include "malbolge/exception.hpp"

#include "test_helpers.hpp"

#include <fstream>

using namespace malbolge;
using namespace debugger;
using namespace literals;

namespace
{
const auto tmp_path = std::filesystem::temp_directory_path() /
                      "debug_script_test.txt";
const auto invalid_tmp_path = std::filesystem::temp_directory_path() /
                              "debug_script_test_invalid.txt";

const auto all_valid_expected = script::functions::sequence{
    script::functions::add_breakpoint{9, 2},
    script::functions::add_breakpoint{42, 2},
    script::functions::add_breakpoint{422343, 2},
    script::functions::add_breakpoint{12012_trit},
    script::functions::add_breakpoint{0x42},
    script::functions::remove_breakpoint{9},
    script::functions::remove_breakpoint{42},
    script::functions::remove_breakpoint{422343},
    script::functions::remove_breakpoint{12012_trit},
    script::functions::remove_breakpoint{0x42},
    script::functions::run{},
    script::functions::run{1},
    script::functions::run{10},
    script::functions::run{0x1},
    script::functions::run{021},
    script::functions::address_value{9},
    script::functions::address_value{42},
    script::functions::address_value{422343},
    script::functions::address_value{12012_trit},
    script::functions::address_value{0x42},
    script::functions::register_value{vcpu_register::A},
    script::functions::register_value{vcpu_register::C},
    script::functions::register_value{vcpu_register::D},
    script::functions::step{},
    script::functions::resume{},
    script::functions::stop{},
    script::functions::on_input{"hello"},
    script::functions::on_input{"he\"llo"}
};

void all_valid_script(std::ostream& stream)
{
    stream << "add_breakpoint(address=9,  ignore_count=2);" << std::endl
           << "add_breakpoint(address=42, ignore_count=0x2);" << std::endl
           << "add_breakpoint(address=422343, ignore_count=0x2);" << std::endl
           << "add_breakpoint(address=t12012);" << std::endl
           << "add_breakpoint(address=0x42);" << std::endl
           << "remove_breakpoint(address=9);" << std::endl
           << "remove_breakpoint(address=42);" << std::endl
           << "remove_breakpoint(address=422343);" << std::endl
           << "remove_breakpoint(address=t12012);" << std::endl
           << "remove_breakpoint(address=0x42);" << std::endl
           << "run();" << std::endl
           << "run(max_runtime_ms=1);" << std::endl
           << "run(max_runtime_ms=10);" << std::endl
           << "run(max_runtime_ms=0x1);" << std::endl
           << "run(max_runtime_ms=021);" << std::endl
           << "address_value(address=9);" << std::endl
           << "address_value(address=42);" << std::endl
           << "address_value(address=422343);" << std::endl
           << "address_value(address=t12012);" << std::endl
           << "address_value(address=0x42);" << std::endl
           << "register_value(reg=A);" << std::endl
           << "register_value(reg=C);" << std::endl
           << "register_value(reg=D);" << std::endl
           << "step();" << std::endl
           << "resume();" << std::endl
           << "stop();" << std::endl
           << "on_input(data=\"hello\");" << std::endl
           << "on_input(data=\"he\\\"llo\");" << std::endl;
}
}

BOOST_AUTO_TEST_SUITE(script_parser_suite)

BOOST_AUTO_TEST_CASE(valid)
{
    auto stream = std::stringstream{};
    all_valid_script(stream);

    const auto result = script::parse(stream);
    BOOST_CHECK_EQUAL(all_valid_expected, result);
}

BOOST_AUTO_TEST_CASE(whitespace_valid)
{
    auto stream = std::stringstream{};
    stream << "add_breakpoint  " << std::endl
           << "\t(\t  address  =  \t9" << std::endl
           << ",  " << std::endl
           << "   \t  ignore_count= 2)" << std::endl
           << "\t\t; run(\t)\t" << std::endl << std::endl << std::endl
           << ";" << std::endl << std::endl << std::endl;

    const auto expected = script::functions::sequence{
        script::functions::add_breakpoint{9, 2},
        script::functions::run{},
    };

    const auto result = script::parse(stream);
    BOOST_CHECK_EQUAL(expected, result);
}

BOOST_AUTO_TEST_CASE(single_invalid)
{
    auto f = [](auto input, auto src_loc) {
        auto stream = std::stringstream{};
        stream << input;

        try {
            script::parse(stream);
            BOOST_CHECK_MESSAGE(false, "Should have thrown");
        } catch (parse_exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            BOOST_REQUIRE(e.location());
            BOOST_CHECK_EQUAL(*(e.location()), src_loc);
        } catch (std::exception& e) {
            BOOST_TEST_MESSAGE(e.what());
            BOOST_CHECK_MESSAGE(false, "Thrown unexpected type");
        }
    };

    test::data_set(
        f,
        {
            // Function name extraction
            std::tuple{"a",                     source_location{1, 1}},
            std::tuple{"  a",                   source_location{1, 3}},
            std::tuple{"\n\n   a",              source_location{3, 4}},
            std::tuple{"  a()",                 source_location{1, 3}},
            std::tuple{"  a\n\t()",             source_location{2, 1}},
            std::tuple{"()",                    source_location{1, 1}},
            std::tuple{"  ()",                  source_location{1, 2}},
            // Argument extraction
            std::tuple{"run(",                  source_location{1, 4}},
            std::tuple{"run(   ",               source_location{1, 7}},
            std::tuple{"run(  \n ",             source_location{2, 1}},
            std::tuple{"run(=9)",               source_location{1, 5}},
            std::tuple{"run( =9)",              source_location{1, 6}},
            std::tuple{"run(max_runtime_ms)",   source_location{1, 18}},
            std::tuple{"run(max_runtime_ms,)",  source_location{1, 19}},
            std::tuple{"run(max_runtime_ms=,)", source_location{1, 20}},
            std::tuple{"add_breakpoint(address=4, ignore_count)",
                       source_location{1, 38}},
            std::tuple{"add_breakpoint(address=4, ignore_count,)",
                       source_location{1, 39}},
            std::tuple{"add_breakpoint(address=4, ignore_count =,)",
                       source_location{1, 41}},
            std::tuple{"on_input(data =\n\t\")",
                       source_location{2, 2}},
            // Function creation
            std::tuple{"run(wrong=100)",        source_location{1, 10}},
            std::tuple{"run(max_runtime_ms=\"hello\")",
                       source_location{1, 19}},
            std::tuple{"add_breakpoint(address=\"hello\")",
                       source_location{1, 23}},
            std::tuple{"register_value(reg=42)",
                       source_location{1, 19}},
        }
    );
}

BOOST_AUTO_TEST_CASE(valid_from_file)
{
    // Try loading a non-existent path
    try {
        script::parse(invalid_tmp_path);
        BOOST_CHECK_MESSAGE(false, "Should have thrown");
    }  catch (basic_exception& e) {
        BOOST_TEST_MESSAGE(e.what());
    }

    // Write to a temporary location
    {
        auto stream = std::ofstream{};
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.open(tmp_path, std::ios::trunc);

        all_valid_script(stream);
    }

    // Load it
    const auto result = script::parse(tmp_path);
    BOOST_CHECK_EQUAL(all_valid_expected, result);
}

BOOST_AUTO_TEST_SUITE_END()
