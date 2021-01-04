/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/exception.hpp"

#include "test_helpers.hpp"

using namespace malbolge;
using namespace std::string_literals;

namespace
{
argument_parser arg_dispatcher(std::vector<std::string> args)
{
    args.insert(args.begin(), "malbolge");

    auto c_args = std::vector<char*>{};
    c_args.reserve(args.size());
    for (auto& arg : args) {
        c_args.push_back(arg.data());
    }

    return argument_parser{static_cast<int>(args.size()),
                           c_args.data()};
}
}

BOOST_AUTO_TEST_SUITE(argument_parser_suite)

BOOST_AUTO_TEST_CASE(no_args)
{
    auto ap = arg_dispatcher({});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
    BOOST_CHECK_EQUAL(ap.program().data, ""s);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());
}

BOOST_AUTO_TEST_CASE(unknown)
{
    try {
        auto ap = arg_dispatcher({"--hello"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(help)
{
    for (auto arg : {"--help", "-h"}) {
        auto ap = arg_dispatcher({arg});

        BOOST_CHECK_EQUAL(ap.help(), true);
        BOOST_CHECK_EQUAL(ap.version(), false);
        BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
        BOOST_CHECK_EQUAL(ap.program().data, ""s);
        BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
        BOOST_CHECK(!ap.force_non_normalised());
        BOOST_CHECK(!ap.debugger_script());
    }
}

BOOST_AUTO_TEST_CASE(version)
{
    for (auto arg : {"--version", "-v"}) {
        auto ap = arg_dispatcher({arg});

        BOOST_CHECK_EQUAL(ap.help(), false);
        BOOST_CHECK_EQUAL(ap.version(), true);
        BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
        BOOST_CHECK_EQUAL(ap.program().data, ""s);
        BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
        BOOST_CHECK(!ap.force_non_normalised());
        BOOST_CHECK(!ap.debugger_script());
    }

    try {
        auto ap = arg_dispatcher({"-h", "-v"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}

    try {
        auto ap = arg_dispatcher({"-v", "-h"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(log_levels)
{
    auto l = 1u;
    for (auto arg : {"-l", "-ll", "-lll"}) {
        auto ap = arg_dispatcher({arg});

        BOOST_CHECK_EQUAL(ap.help(), false);
        BOOST_CHECK_EQUAL(ap.version(), false);
        BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
        BOOST_CHECK_EQUAL(ap.program().data, ""s);
        BOOST_CHECK_EQUAL(ap.log_level(), static_cast<log::level>(log::ERROR-l));
        BOOST_CHECK(!ap.force_non_normalised());
        BOOST_CHECK(!ap.debugger_script());

        ++l;
    }

    try {
        auto ap = arg_dispatcher({"-v", "-l"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}

    try {
        auto ap = arg_dispatcher({"-llll"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}

    try {
        auto ap = arg_dispatcher({"-lab"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(force_nn)
{
    auto ap = arg_dispatcher({"--force-non-normalised"});
    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
    BOOST_CHECK_EQUAL(ap.program().data, ""s);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());
}

BOOST_AUTO_TEST_CASE(file)
{
    const auto path = "/home/user/anon/prog.mal"s;
    auto ap = arg_dispatcher({path});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::DISK);
    BOOST_CHECK_EQUAL(ap.program().data, path);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());

    try {
        auto ap = arg_dispatcher({path, "-l"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}

    try {
        auto ap = arg_dispatcher({"--string", "This will not compile", path});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}

    try {
        auto ap = arg_dispatcher({path, "--string", "This will not compile", });
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(string)
{
    const auto source = "This will not compile"s;
    auto ap = arg_dispatcher({"--string", source});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STRING);
    BOOST_CHECK_EQUAL(ap.program().data, source);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());

    try {
        auto ap = arg_dispatcher({"--string"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(logging_and_file)
{
    const auto path = "/home/user/anon/prog.mal";
    auto ap = arg_dispatcher({"-ll", path});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::DISK);
    BOOST_CHECK_EQUAL(ap.program().data, path);
    BOOST_CHECK_EQUAL(ap.log_level(), log::DEBUG);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());
}

BOOST_AUTO_TEST_CASE(logging_and_string)
{
    const auto source = "This will not compile"s;
    auto ap = arg_dispatcher({"-ll", "--string", source});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STRING);
    BOOST_CHECK_EQUAL(ap.program().data, source);
    BOOST_CHECK_EQUAL(ap.log_level(), log::DEBUG);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_CHECK(!ap.debugger_script());
}

BOOST_AUTO_TEST_CASE(debugger_script)
{
    const auto script = "/path/to/script"s;
    auto ap = arg_dispatcher({"--debugger-script", script});
    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::STDIN);
    BOOST_CHECK_EQUAL(ap.program().data, ""s);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_REQUIRE(ap.debugger_script());
    BOOST_CHECK_EQUAL(*(ap.debugger_script()), script);
}

BOOST_AUTO_TEST_CASE(debugger_script_and_file)
{
    const auto file_path = "/home/user/anon/prog.mal";
    const auto script = "/path/to/script"s;
    auto ap = arg_dispatcher({"--debugger-script", script, file_path});
    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_CHECK_EQUAL(ap.program().source, argument_parser::program_source::DISK);
    BOOST_CHECK_EQUAL(ap.program().data, file_path);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    BOOST_CHECK(!ap.force_non_normalised());
    BOOST_REQUIRE(ap.debugger_script());
    BOOST_CHECK_EQUAL(*(ap.debugger_script()), script);
}

BOOST_AUTO_TEST_SUITE_END()
