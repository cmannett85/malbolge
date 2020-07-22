/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/exception.hpp"

#include "test_helpers.hpp"

using namespace malbolge;

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
    BOOST_CHECK_EQUAL(ap.file().has_value(), false);
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
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
        BOOST_CHECK_EQUAL(ap.file().has_value(), false);
        BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
    }
}

BOOST_AUTO_TEST_CASE(version)
{
    for (auto arg : {"--version", "-v"}) {
        auto ap = arg_dispatcher({arg});

        BOOST_CHECK_EQUAL(ap.help(), false);
        BOOST_CHECK_EQUAL(ap.version(), true);
        BOOST_CHECK_EQUAL(ap.file().has_value(), false);
        BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);
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
        BOOST_CHECK_EQUAL(ap.file().has_value(), false);
        BOOST_CHECK_EQUAL(ap.log_level(), static_cast<log::level>(log::ERROR-l));

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

BOOST_AUTO_TEST_CASE(file)
{
    const auto path = "/home/user/anon/prog.mal";
    auto ap = arg_dispatcher({path});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_REQUIRE(ap.file().has_value());
    BOOST_CHECK_EQUAL(*ap.file(), std::filesystem::path{path});
    BOOST_CHECK_EQUAL(ap.log_level(), log::ERROR);

    try {
        auto ap = arg_dispatcher({path, "-l"});
        BOOST_FAIL("Should have thrown");
    } catch (system_exception&) {}
}

BOOST_AUTO_TEST_CASE(logging_and_file)
{
    const auto path = "/home/user/anon/prog.mal";
    auto ap = arg_dispatcher({"-ll", path});

    BOOST_CHECK_EQUAL(ap.help(), false);
    BOOST_CHECK_EQUAL(ap.version(), false);
    BOOST_REQUIRE(ap.file().has_value());
    BOOST_CHECK_EQUAL(*ap.file(), std::filesystem::path{path});
    BOOST_CHECK_EQUAL(ap.log_level(), log::DEBUG);
}

BOOST_AUTO_TEST_SUITE_END()
