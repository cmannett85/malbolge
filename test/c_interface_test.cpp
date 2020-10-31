/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/c_interface.hpp"
#include "malbolge/version.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/virtual_memory.hpp"

#include "test_helpers.hpp"

#include <filesystem>
#include <iterator>
#include <fstream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace malbolge;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace
{
std::vector<char> load_program_from_disk(const std::filesystem::path& path)
{
    const auto file_size = std::filesystem::file_size(path);
    auto data = std::vector<char>{};
    data.reserve(file_size);

    {
        auto stream = std::ifstream{};
        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.open(path, std::ios::binary);

        std::copy(std::istreambuf_iterator<char>{stream},
                  std::istreambuf_iterator<char>{},
                  std::back_inserter(data));
    }

    return data;
}

class fixture
{
public:
    fixture()
    {
        vcpu = nullptr;
        stopped = false;
        waiting = false;
    }

    static void stopped_cb(int err, malbolge_virtual_cpu handle)
    {
        BOOST_CHECK_EQUAL(err, 0);
        BOOST_CHECK_EQUAL(handle, vcpu);
        stopped = true;
    }

    static void waiting_cb(malbolge_virtual_cpu handle)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        waiting = true;
    }

    static malbolge_virtual_cpu vcpu;
    static bool stopped;
    static bool waiting;
};

auto expected_address = 0u;
auto callback_reached = false;
auto dbg_cv = std::condition_variable{};
auto dbg_mtx = std::mutex{};
int breakpoint_callback(unsigned int address,
                        malbolge_debugger_vcpu_register_id reg)
{
    BOOST_CHECK_EQUAL(address, expected_address);
    BOOST_CHECK_EQUAL(reg, MALBOLGE_DBG_REGISTER_C);

    {
        auto lock = std::lock_guard{dbg_mtx};
        callback_reached = true;
    }
    dbg_cv.notify_one();

    return MALBOLGE_ERR_TRUE;
};

malbolge_virtual_cpu fixture::vcpu;
bool fixture::stopped;
bool fixture::waiting;
}

BOOST_AUTO_TEST_SUITE(c_interface_suite)

BOOST_AUTO_TEST_CASE(log_level)
{
    auto f = [](auto level, auto success) {
        const auto r = malbolge_set_log_level(level);
        BOOST_CHECK_EQUAL(r, success);

        if (!success) {
            const auto lvl = malbolge_log_level();
            BOOST_CHECK_EQUAL(lvl, level);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{0, MALBOLGE_ERR_SUCCESS},
            std::tuple{1, MALBOLGE_ERR_SUCCESS},
            std::tuple{2, MALBOLGE_ERR_SUCCESS},
            std::tuple{3, MALBOLGE_ERR_SUCCESS},
            std::tuple{4, MALBOLGE_ERR_INVALID_LOG_LEVEL},
        }
    );
}

BOOST_AUTO_TEST_CASE(version)
{
    BOOST_CHECK_EQUAL(std::strcmp(malbolge_version(), version_string), 0);
}

BOOST_AUTO_TEST_CASE(is_likely_normalised_test)
{
    auto f = [](auto&& source, auto&& expected) {
        const auto r = malbolge_is_likely_normalised_source(source.data(),
                                                            source.size());
        BOOST_CHECK_EQUAL(r, expected);
    };

    test::data_set(
        f,
        {
            std::tuple{"jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       1},
            std::tuple{R"_(('&%#^"!~}{XE)_"s,
                       0},
            std::tuple{"jjjjjjjjjjjjjjjjdjjjjjjj*<jjjjjjjjjjjjjjjjjjjjjjjj*<v"s,
                       0},
            std::tuple{""s,
                       1},
        }
    );
}

BOOST_AUTO_TEST_CASE(normalise_source_test)
{
    auto f = [](auto source, auto&& expected, auto loc, auto fail) {
        auto fail_line = 0u;
        auto fail_column = 0u;
        auto new_size = 0ul;
        const auto r = malbolge_normalise_source(source.data(),
                                                 source.size(),
                                                 &new_size,
                                                 loc ? &fail_line : nullptr,
                                                 loc ? &fail_column : nullptr);
        BOOST_CHECK_EQUAL(r != 0, fail);

        if (r == 0) {
            if (new_size < source.size()) {
                BOOST_CHECK_EQUAL(source[new_size], '\n');
            }
            source.resize(new_size);
            BOOST_CHECK_EQUAL(source, expected);
        }

        if (loc) {
            BOOST_CHECK(fail);
            BOOST_CHECK_EQUAL(loc->line, fail_line);
            BOOST_CHECK_EQUAL(loc->column, fail_column);
        } else {
            BOOST_CHECK_EQUAL(0, fail_line);
            BOOST_CHECK_EQUAL(0, fail_column);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{R"_((=BA#9"=<;:3y7x54-21q/p-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       "jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       optional_source_location{},
                       false},
            std::tuple{R"_((=BA#9"=<;:3y7x54- 21q/p-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       "jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       optional_source_location{},
                       false},
            std::tuple{R"_(('&%#^"!~f}{XE)_"s,
                       ""s,
                       optional_source_location{},
                       true},
            std::tuple{R"_(('&%#^"!~f}{XE)_"s,
                       ""s,
                       optional_source_location{source_location{1, 10}},
                       true},
        }
    );

    // Null tests
    auto new_size = 0ul;
    auto r = malbolge_normalise_source(nullptr, 0u, &new_size, nullptr, nullptr);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_NULL_ARG);

    auto source = ""s;
    r = malbolge_normalise_source(source.data(), 0u, nullptr, nullptr, nullptr);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_NULL_ARG);

    r = malbolge_normalise_source(nullptr, 0u, nullptr, nullptr, nullptr);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_NULL_ARG);
}

BOOST_AUTO_TEST_CASE(denormalise_source_test)
{
    auto f = [](auto source, auto&& expected, auto loc, auto fail) {
        auto fail_column = 0u;
        const auto r = malbolge_denormalise_source(source.data(),
                                                   source.size(),
                                                   loc ? &fail_column : nullptr);
        BOOST_CHECK_EQUAL(r != 0, fail);

        if (r == 0) {
            BOOST_CHECK_EQUAL(source, expected);
        }

        if (loc) {
            BOOST_CHECK(fail);
            BOOST_CHECK_EQUAL(loc->column, fail_column);
        } else {
            BOOST_CHECK_EQUAL(0, fail_column);
        }
    };

    test::data_set(
        f,
        {
            std::tuple{"jpoo*pjoooop*ojoopoo*ojoooooppjoivvvo/i<ivivi<vvvvvvvvvvvvvoji"s,
                       R"_((=BA#9"=<;:3y7x54-21q/p-,+*)"!h%B0/.~P<<:(8&66#"!~}|{zyxwvugJ%)_"s,
                       optional_source_location{},
                       false},
            std::tuple{"jjjj*<jjfjj*<v"s,
                       ""s,
                       optional_source_location{},
                       true},
            std::tuple{"jjjj*<jjfjj*<v"s,
                       ""s,
                       optional_source_location{source_location{1, 9}},
                       true},
        }
    );

    // Null tests
    auto r = malbolge_denormalise_source(nullptr, 0u, nullptr);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_NULL_ARG);
}

BOOST_AUTO_TEST_CASE(load)
{
    using pdata_t = std::tuple<std::vector<char>,
                               std::vector<char>,
                               optional_source_location,
                               bool>;

    auto f = [](auto program_data, auto parse_data, auto loc, auto fail) {
        auto fail_line = 0u;
        auto fail_column = 0u;
        const auto r = malbolge_load_program(program_data.data(),
                                             program_data.size(),
                                             loc ? &fail_line : nullptr,
                                             loc ? &fail_column : nullptr);
        BOOST_CHECK_EQUAL(!r, fail);

        if (r) {
            const auto& vmem = *static_cast<virtual_memory*>(r);
            for (auto i = 0u; i < parse_data.size(); ++i) {
                BOOST_CHECK_EQUAL(parse_data[i], vmem[i]);
            }
        }

        if (loc) {
            BOOST_CHECK(fail);
            BOOST_CHECK_EQUAL(loc->line, fail_line);
            BOOST_CHECK_EQUAL(loc->column, fail_column);
        } else {
            BOOST_CHECK_EQUAL(0, fail_line);
            BOOST_CHECK_EQUAL(0, fail_column);
        }
    };

    test::data_set(
        f,
        {
            pdata_t{{},
                    {},
                    {},
                    true},
            pdata_t{{40},
                    {},
                    {},
                    true},
            pdata_t{{40, 39},
                    {40, 39},
                    {},
                    false},
            pdata_t{{' ', '\0'},
                    {},
                    source_location{1, 2},
                    true},
            pdata_t{{' ', '\t', '\n'},
                    {},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n', 40},
                    {40},
                    {},
                    true},
            pdata_t{{' ', '\t', '\n', 40, 39},
                    {40, 39},
                    {},
                    false},
        }
    );
}

BOOST_FIXTURE_TEST_CASE(vcpu_nulls, fixture)
{
    {
        const auto bad_vcpu = malbolge_create_vcpu(nullptr);
        BOOST_CHECK(!bad_vcpu);
    }

    {
        const auto result = malbolge_vcpu_run(nullptr, stopped_cb, waiting_cb, 1);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
    }

    {
        const auto result = malbolge_vcpu_stop(nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
    }

    {
        auto buffer = "hello";
        auto result = malbolge_vcpu_input(nullptr, buffer, 6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        result = malbolge_vcpu_input(reinterpret_cast<malbolge_virtual_cpu*>(42),
                                     nullptr,
                                     6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        result = malbolge_vcpu_input(nullptr, nullptr, 6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
    }
}

// These next tests kinda stink because I can't test the cout output without
// somewhat ruining the simplicity of the C interface (i.e. I can't redirect
// cout because there are no iostreams in C).  Thankfully the underlying C++
// code is fully tested, so it's not as bad as it looks
BOOST_FIXTURE_TEST_CASE(run_hello_world, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    const auto result = malbolge_vcpu_run(vcpu, stopped_cb, waiting_cb, 1);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(stopped);

    const auto r = malbolge_vcpu_input(vcpu, "Goodbye!", 8);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_CIN_OR_STOPPED);
}

BOOST_FIXTURE_TEST_CASE(run_hello_world_string, fixture)
{
    auto buffer = R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>
)"s;
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    const auto result = malbolge_vcpu_run(vcpu, stopped_cb, waiting_cb, 1);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(stopped);

    const auto r = malbolge_vcpu_input(vcpu, "Goodbye!", 8);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_CIN_OR_STOPPED);
}

BOOST_FIXTURE_TEST_CASE(run_hello_world_string_normalised, fixture)
{
    auto buffer = "jjjjpp<jjjj*p<jjjpp<<jjjj*p<jj*o*<i<io<</<<oo<*o*<jvoo<<opj<*<<<<<ojjopjp<jio<ovo<<jo<p*o<*jo<iooooo<jj*p<jji<oo<j*jp<jj**p<jjopp<i"s;
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_normalised_program(buffer.data(),
                                                 buffer.size(),
                                                 &fail_line,
                                                 &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    const auto result = malbolge_vcpu_run(vcpu, stopped_cb, waiting_cb, 1);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(stopped);

    const auto r = malbolge_vcpu_input(vcpu, "Goodbye!", 8);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_CIN_OR_STOPPED);
}

BOOST_FIXTURE_TEST_CASE(run_echo, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/echo.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    const auto result = malbolge_vcpu_run(vcpu, stopped_cb, waiting_cb, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(waiting);
    waiting = false;
    auto r = malbolge_vcpu_input(vcpu, "Hello!", 6);
    BOOST_CHECK_EQUAL(r, 0);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(waiting);
    waiting = false;
    r = malbolge_vcpu_input(vcpu, "Goodbye!", 8);

    // Wait a moment so we are back into waiting for input (to test stopping)
    // Issue #97
    std::this_thread::sleep_for(100ms);
    malbolge_vcpu_stop(vcpu);
    BOOST_CHECK_EQUAL(r, 0);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(stopped);
}

BOOST_FIXTURE_TEST_CASE(run_hello_world_debugger, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto dbg = malbolge_debugger_attach(nullptr);
    BOOST_CHECK(!dbg);
    dbg = malbolge_debugger_attach(vcpu);
    BOOST_REQUIRE(dbg);

    expected_address = 9;
    callback_reached = false;
    auto result = malbolge_debugger_add_breakpoint(nullptr,
                                                   expected_address,
                                                   breakpoint_callback,
                                                   0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_debugger_add_breakpoint(dbg,
                                              expected_address,
                                              breakpoint_callback,
                                              0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    result = malbolge_debugger_add_breakpoint(dbg,
                                              expected_address + 10,
                                              breakpoint_callback,
                                              0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    result = malbolge_debugger_add_breakpoint(dbg,
                                              expected_address + 11,
                                              breakpoint_callback,
                                              0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    result = malbolge_vcpu_run(vcpu, stopped_cb, waiting_cb, 1);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        // BP1
        {
            auto lock = std::unique_lock{dbg_mtx};
            dbg_cv.wait(lock, [&]() { return callback_reached; });
        }
        std::this_thread::sleep_for(100ms);
        result = malbolge_debugger_address_value(nullptr, expected_address);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_register_value(nullptr,
                                                  MALBOLGE_DBG_REGISTER_A,
                                                  nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_address_value(dbg, expected_address);
        BOOST_CHECK_EQUAL(result, 125);
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_A,
                                                  nullptr);
        BOOST_CHECK_EQUAL(result, 72);
        auto reg_addr = 0u;
        auto reg_addr_ptr = &reg_addr;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_C,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 125);
        BOOST_CHECK_EQUAL(reg_addr, 9);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_D,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 37);
        BOOST_CHECK_EQUAL(reg_addr, 62);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_A,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 72);
        BOOST_CHECK_EQUAL(reg_addr_ptr, nullptr);
        reg_addr_ptr = &reg_addr;

        result = malbolge_debugger_step(nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_step(dbg);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        std::this_thread::sleep_for(100ms);
        result = malbolge_debugger_address_value(dbg, expected_address+1);
        BOOST_CHECK_EQUAL(result, 124);
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_A,
                                                  nullptr);
        BOOST_CHECK_EQUAL(result, 72);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_C,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 124);
        BOOST_CHECK_EQUAL(reg_addr, 10);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_D,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 61);
        BOOST_CHECK_EQUAL(reg_addr, 38);

        // BP2
        callback_reached = false;
        expected_address += 10;
        result = malbolge_debugger_resume(nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_resume(dbg);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{dbg_mtx};
            dbg_cv.wait(lock, [&]() { return callback_reached; });
        }
        std::this_thread::sleep_for(100ms);
        result = malbolge_debugger_address_value(dbg, expected_address);
        BOOST_CHECK_EQUAL(result, 80);
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_A,
                                                  nullptr);
        BOOST_CHECK_EQUAL(result, 9836);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_C,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 80);
        BOOST_CHECK_EQUAL(reg_addr, 19);
        reg_addr = 0u;
        result = malbolge_debugger_register_value(dbg,
                                                  MALBOLGE_DBG_REGISTER_D,
                                                  &reg_addr_ptr);
        BOOST_CHECK_EQUAL(result, 125);
        BOOST_CHECK_EQUAL(reg_addr, 37);

        callback_reached = false;
        result = malbolge_debugger_remove_breakpoint(nullptr, expected_address + 1);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_remove_breakpoint(dbg, expected_address + 1);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_TRUE);
        result = malbolge_debugger_resume(dbg);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

        result = malbolge_debugger_pause(nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_debugger_pause(dbg);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        std::this_thread::sleep_for(100ms);
        result = malbolge_debugger_resume(dbg);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        BOOST_CHECK(!callback_reached);
    }

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(stopped);
    BOOST_CHECK(!callback_reached);

    const auto r = malbolge_vcpu_input(vcpu, "Goodbye!", 8);
    BOOST_CHECK_EQUAL(r, MALBOLGE_ERR_CIN_OR_STOPPED);
}

BOOST_AUTO_TEST_SUITE_END()
