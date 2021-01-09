/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/version.hpp"

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
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({"-v", "-h"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({"-llll"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({"-lab"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({"--string", "This will not compile", path});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({path, "--string", "This will not compile"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }

    try {
        auto ap = arg_dispatcher({path, "/another/path"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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

    try {
        auto ap = arg_dispatcher({"--debugger-script"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception& e) {
        BOOST_CHECK_EQUAL(e.code().value(),
                          static_cast<int>(std::errc::invalid_argument));
    }
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

BOOST_AUTO_TEST_CASE(program_source_streaming_operator)
{
    auto f = [](auto source, auto expected) {
        auto ss = std::stringstream{};
        ss << source;
        BOOST_CHECK_EQUAL(ss.str(), expected);
    };

    test::data_set(
        f,
        {
            std::tuple{argument_parser::program_source::DISK,   "DISK"},
            std::tuple{argument_parser::program_source::STDIN,  "STDIN"},
            std::tuple{argument_parser::program_source::STRING, "STRING"},
            std::tuple{argument_parser::program_source::MAX,    "Unknown"},
        }
    );
}

BOOST_AUTO_TEST_CASE(argument_parser_streaming_operator)
{
    const auto expected = "Malbolge virtual machine v"s + project_version +
        "\nUsage:"
        "\n\tmalbolge [options] <file>\n"
        "\tcat <file> | malbolge [options]\n\n"
        "Options:\n"
        "\t-h --help\t\tDisplay this help message\n"
        "\t-v --version\t\tDisplay the full application version\n"
        "\t-l\t\t\tLog level, repeat the l character for higher logging levels\n"
        "\t--string\t\tPass a string argument as the program to run\n"
        "\t--debugger-script\tRun the given debugger script on the program\n"
        "\t--force-non-normalised\tOverride normalised program detection to force to non-normalised";

    auto ss = std::stringstream{};
    ss << arg_dispatcher({});
    BOOST_CHECK_EQUAL(ss.str(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
