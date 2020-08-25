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

using namespace malbolge;
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

malbolge_virtual_cpu global_vcpu = nullptr;
auto global_stopped = false;
void stopped_cb(int err, malbolge_virtual_cpu handle)
{
    BOOST_CHECK_EQUAL(err, 0);
    BOOST_CHECK_EQUAL(handle, global_vcpu);
    global_stopped = true;
}

auto global_waiting = false;
void waiting_cb(malbolge_virtual_cpu handle)
{
    BOOST_CHECK_EQUAL(handle, global_vcpu);
    global_waiting = true;
}
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
            std::tuple{0, 0},
            std::tuple{1, 0},
            std::tuple{2, 0},
            std::tuple{3, 0},
            std::tuple{4, -EINVAL},
        }
    );
}

BOOST_AUTO_TEST_CASE(version)
{
    BOOST_CHECK_EQUAL(std::strcmp(malbolge_version(), version_string), 0);
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

// These next two tests kinda stink because I can't test the cout output
// without somewhat ruining the simplicity of the C interface (i.e. I can't
// redirect cout because there are no iostreams in C).  Thankfully the
// underlying C++ code is fully tested, so it's not too bad
BOOST_AUTO_TEST_CASE(run_hello_world)
{
    malbolge_set_log_level(1);

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

    global_stopped = false;
    global_vcpu = malbolge_vcpu_run(vmem, stopped_cb, waiting_cb, 1);
    BOOST_REQUIRE(global_vcpu);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(global_stopped);

    const auto r = malbolge_vcpu_input(global_vcpu, "Goodbye!", 8);
    BOOST_CHECK_NE(r, 0);
}

BOOST_AUTO_TEST_CASE(run_echo)
{
    malbolge_set_log_level(1);

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

    global_stopped = false;
    global_waiting = true;
    global_vcpu = malbolge_vcpu_run(vmem, stopped_cb, waiting_cb, 0);
    BOOST_REQUIRE(global_vcpu);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(global_waiting);
    global_waiting = false;
    auto r = malbolge_vcpu_input(global_vcpu, "Hello!", 6);
    BOOST_CHECK_EQUAL(r, 0);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(global_waiting);
    global_waiting = false;
    r = malbolge_vcpu_input(global_vcpu, "Goodbye!", 8);

    // Wait a moment so we are back into waiting for input (to test stopping)
    // Issue #97
    std::this_thread::sleep_for(100ms);
    malbolge_vcpu_stop(global_vcpu);
    BOOST_CHECK_EQUAL(r, 0);

    std::this_thread::sleep_for(100ms);
    BOOST_CHECK(global_stopped);
}

BOOST_AUTO_TEST_SUITE_END()
