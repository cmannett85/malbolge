/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/c_interface.hpp"
#include "malbolge/version.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/virtual_memory.hpp"
#include "malbolge/log.hpp"

#include "test_helpers.hpp"

#include <filesystem>
#include <iterator>
#include <fstream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <deque>

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
        paused = false;
        waiting = false;
        running = false;
        bp_hit = false;
        value_query_hit = false;
        expected_states.clear();
        expected_ec = 0;
        output_str.clear();
        expected_address = 0;
        expected_value = 0;
        expected_reg = MALBOLGE_VCPU_REGISTER_MAX;
    }

    static void state_cb(malbolge_virtual_cpu handle,
                         malbolge_vcpu_execution_state state,
                         int ec)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        BOOST_REQUIRE(!expected_states.empty());
        BOOST_CHECK_EQUAL(state, expected_states.front());
        BOOST_CHECK_EQUAL(ec, expected_ec);

        expected_states.pop_front();

        {
            auto lock = std::lock_guard{mtx};
            if (state == MALBOLGE_VCPU_WAITING_FOR_INPUT) {
                waiting = true;
            } else if (state == MALBOLGE_VCPU_STOPPED) {
                stopped = true;
            } else if (state == MALBOLGE_VCPU_PAUSED) {
                paused = true;
            } else if (state == MALBOLGE_VCPU_RUNNING) {
                running = true;
            }
        }
        cv.notify_all();
    }

    static void output_cb(malbolge_virtual_cpu handle, char c)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        output_str += c;
    }

    static void breakpoint_cb(malbolge_virtual_cpu handle, unsigned int address)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        BOOST_CHECK_EQUAL(address, expected_address);

        {
            auto lock = std::lock_guard{mtx};
            bp_hit = true;
        }
        cv.notify_all();
    }

    static void address_value_cb(malbolge_virtual_cpu handle,
                                 unsigned int address,
                                 unsigned int value)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        BOOST_CHECK_EQUAL(address, expected_address);
        BOOST_CHECK_EQUAL(value, expected_value);

        {
            auto lock = std::lock_guard{mtx};
            value_query_hit = true;
        }
        cv.notify_all();
    }

    static void register_value_cb(malbolge_virtual_cpu handle,
                                  enum malbolge_vcpu_register reg,
                                  unsigned int address,
                                  unsigned int value)
    {
        BOOST_CHECK_EQUAL(handle, vcpu);
        BOOST_CHECK_EQUAL(reg, expected_reg);
        BOOST_CHECK_EQUAL(address, expected_address);
        BOOST_CHECK_EQUAL(value, expected_value);

        {
            auto lock = std::lock_guard{mtx};
            value_query_hit = true;
        }
        cv.notify_all();
    }

    static malbolge_virtual_cpu vcpu;
    static std::mutex mtx;
    static std::condition_variable cv;
    static bool stopped;
    static bool paused;
    static bool waiting;
    static bool running;
    static bool bp_hit;
    static bool value_query_hit;

    static std::deque<malbolge_vcpu_execution_state> expected_states;
    static int expected_ec;

    static std::string output_str;

    static unsigned int expected_address;
    static unsigned int expected_value;
    static malbolge_vcpu_register expected_reg;
};

malbolge_virtual_cpu fixture::vcpu;
bool fixture::stopped;
bool fixture::paused;
bool fixture::waiting;
bool fixture::running;
bool fixture::bp_hit;
bool fixture::value_query_hit;
std::mutex fixture::mtx;
std::condition_variable fixture::cv;
std::deque<malbolge_vcpu_execution_state> fixture::expected_states;
int fixture::expected_ec;
std::string fixture::output_str;
unsigned int fixture::expected_address;
unsigned int fixture::expected_value;
malbolge_vcpu_register fixture::expected_reg;
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

    // Set it back to the default
    log::set_log_level(log::INFO);
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

    // Null check
    const auto result = malbolge_is_likely_normalised_source(nullptr, 42);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
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
                                             MALBOLGE_LOAD_NORMALISED_AUTO,
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

    auto result = malbolge_vcpu_attach_callbacks(nullptr,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_detach_callbacks(nullptr,
                                            state_cb,
                                            output_cb,
                                            breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_run(nullptr);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_pause(nullptr);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_step(nullptr);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    {
        auto buffer = "hello";
        auto result = malbolge_vcpu_add_input(nullptr, buffer, 6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        result = malbolge_vcpu_add_input(reinterpret_cast<malbolge_virtual_cpu*>(42),
                                         nullptr,
                                         6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        result = malbolge_vcpu_add_input(nullptr, nullptr, 6);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
    }

    result = malbolge_vcpu_add_breakpoint(nullptr, 0, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_remove_breakpoint(nullptr, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_address_value(nullptr, 0, address_value_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

    result = malbolge_vcpu_register_value(nullptr, MALBOLGE_VCPU_REGISTER_A, register_value_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
}

BOOST_FIXTURE_TEST_CASE(hello_world, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_STOPPED
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK(expected_states.empty());

    result = malbolge_vcpu_detach_callbacks(vcpu,
                                            state_cb,
                                            output_cb,
                                            breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    // Attempting to run, pause, or step, after it has been stopped should fail
    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_EXECUTION_FAIL);
    result = malbolge_vcpu_pause(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_EXECUTION_FAIL);
    result = malbolge_vcpu_step(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_EXECUTION_FAIL);
}

BOOST_FIXTURE_TEST_CASE(hello_world_detached, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    result = malbolge_vcpu_detach_callbacks(vcpu,
                                            nullptr,
                                            output_cb,
                                            nullptr);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_STOPPED
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(hello_world_string, fixture)
{
    auto buffer = R"(('&%:9]!~}|z2Vxwv-,POqponl$Hjig%eB@@>}=<M:9wv6WsU2T|nm-,jcL(I&%$#"`CB]V?Tx<uVtT`Rpo3NlF.Jh++FdbCBA@?]!~|4XzyTT43Qsqq(Lnmkj"Fhg${z@>
)"s;
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_STOPPED
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(hello_world_string_normalised, fixture)
{
    auto buffer = "jjjjpp<jjjj*p<jjjpp<<jjjj*p<jj*o*<i<io<</<<oo<*o*<jvoo<<opj<*<<<<<ojjopjp<jio<ovo<<jo<p*o<*jo<iooooo<jj*p<jji<oo<j*jp<jj**p<jjopp<i"s;
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_STOPPED
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(echo, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/echo.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_WAITING_FOR_INPUT,
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_WAITING_FOR_INPUT,
        MALBOLGE_VCPU_STOPPED
    };

    // Adding these should NOT automatically start the program
    result = malbolge_vcpu_add_input(vcpu, "Hello!\n", 8);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(!cv.wait_for(lock, 100ms, [&]() { return running; }));
    }
    BOOST_CHECK(!waiting);

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return waiting; }));
        waiting = false;
    }
    result = malbolge_vcpu_add_input(vcpu, "Goodbye!", 9);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return waiting; }));
    }

    // Attempting to run, pause, or step is a no-op
    running = false;
    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(!cv.wait_for(lock, 100ms, [&]() { return running; }));
    }

    result = malbolge_vcpu_pause(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(!cv.wait_for(lock, 100ms, [&]() { return paused; }));
    }

    paused = false;
    result = malbolge_vcpu_step(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(!cv.wait_for(lock, 100ms, [&]() { return paused; }));
    }

    malbolge_free_vcpu(vcpu);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }

    BOOST_CHECK_EQUAL(output_str, "Hello!\nGoodbye!");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(invalid_register_value_query, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_address = 0;
    result = malbolge_vcpu_add_breakpoint(vcpu, expected_address, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_PAUSED,     // BP
        MALBOLGE_VCPU_STOPPED,    // Stopped
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return bp_hit; }));
    }

    value_query_hit = false;
    expected_ec = MALBOLGE_ERR_EXECUTION_FAIL;
    result = malbolge_vcpu_register_value(vcpu,
                                          MALBOLGE_VCPU_REGISTER_MAX,
                                          register_value_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(!cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
    }

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(hello_world_debugger, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/hello_world.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_address = 9;
    result = malbolge_vcpu_add_breakpoint(vcpu, expected_address, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    result = malbolge_vcpu_add_breakpoint(vcpu, expected_address + 10, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    result = malbolge_vcpu_add_breakpoint(vcpu, expected_address + 11, 0);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_PAUSED,     // BP1, Step
        MALBOLGE_VCPU_RUNNING,    // Resume
        MALBOLGE_VCPU_PAUSED,     // BP2
        MALBOLGE_VCPU_RUNNING,    // Resume
        MALBOLGE_VCPU_PAUSED,     // Pause
        MALBOLGE_VCPU_RUNNING,    // Resume
        MALBOLGE_VCPU_STOPPED,    // Stopped
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        // BP1
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return bp_hit; }));
        }
        bp_hit = false;
        paused = false;

        // Callback NULL checks
        result = malbolge_vcpu_address_value(vcpu, 0, nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_vcpu_address_value(nullptr, 0, nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        result = malbolge_vcpu_register_value(vcpu, MALBOLGE_VCPU_REGISTER_A, nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);
        result = malbolge_vcpu_register_value(nullptr, MALBOLGE_VCPU_REGISTER_A, nullptr);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_NULL_ARG);

        // Register and address checks
        expected_value = 125;
        result = malbolge_vcpu_address_value(vcpu, 9, address_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_A;
        expected_address = 0;
        expected_value = 72;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_A,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_C;
        expected_address = 9;
        expected_value = 125;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_C,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_D;
        expected_address = 62;
        expected_value = 37;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_D,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        // Step
        result = malbolge_vcpu_step(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        paused = false;

        value_query_hit = false;
        expected_address = 10;
        expected_value = 124;
        result = malbolge_vcpu_address_value(vcpu, 10, address_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_A;
        expected_address = 0;
        expected_value = 72;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_A,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_C;
        expected_address = 10;
        expected_value = 124;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_C,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_D;
        expected_address = 38;
        expected_value = 61;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_D,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        // Resume
        expected_address = 19;
        result = malbolge_vcpu_run(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

        // BP2
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return bp_hit; }));
        }
        bp_hit = false;
        paused = false;

        value_query_hit = false;
        expected_address = 19;
        expected_value = 80;
        result = malbolge_vcpu_address_value(vcpu, 19, address_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_A;
        expected_address = 0;
        expected_value = 9836;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_A,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_C;
        expected_address = 19;
        expected_value = 80;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_C,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        value_query_hit = false;
        expected_reg = MALBOLGE_VCPU_REGISTER_D;
        expected_address = 37;
        expected_value = 125;
        result = malbolge_vcpu_register_value(vcpu,
                                              MALBOLGE_VCPU_REGISTER_D,
                                              register_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }

        result = malbolge_vcpu_remove_breakpoint(vcpu, 20);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

        // Resume and then immediately pause.  The now removed BP3 should not
        // fire
        result = malbolge_vcpu_run(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        result = malbolge_vcpu_pause(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return paused; }));
        }

        // Resume to finish
        result = malbolge_vcpu_run(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    }

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "Hello World!");
    BOOST_CHECK(expected_states.empty());
}

BOOST_FIXTURE_TEST_CASE(debugger_ignore_count, fixture)
{
    auto buffer = load_program_from_disk(std::filesystem::path{"programs/echo.mal"});
    auto fail_line = 0u;
    auto fail_column = 0u;
    auto vmem = malbolge_load_program(buffer.data(),
                                      buffer.size(),
                                      MALBOLGE_LOAD_NORMALISED_AUTO,
                                      &fail_line,
                                      &fail_column);
    BOOST_REQUIRE(vmem);
    BOOST_CHECK_EQUAL(fail_line, 0);
    BOOST_CHECK_EQUAL(fail_column, 0);

    vcpu = malbolge_create_vcpu(vmem);
    BOOST_REQUIRE(vcpu);

    auto result = malbolge_vcpu_attach_callbacks(vcpu,
                                                 state_cb,
                                                 output_cb,
                                                 breakpoint_cb);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_address = 37;
    result = malbolge_vcpu_add_breakpoint(vcpu, expected_address, 17);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    result = malbolge_vcpu_add_input(vcpu, "a", 2);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

    expected_states = {
        MALBOLGE_VCPU_RUNNING,
        MALBOLGE_VCPU_PAUSED,               // BP
        MALBOLGE_VCPU_RUNNING,              // Resume
        MALBOLGE_VCPU_PAUSED,               // BP
        MALBOLGE_VCPU_RUNNING,              // Resume
        MALBOLGE_VCPU_WAITING_FOR_INPUT,
        MALBOLGE_VCPU_STOPPED,              // Stopped
    };

    result = malbolge_vcpu_run(vcpu);
    BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
    {
        // BP
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return bp_hit; }));
        }
        bp_hit = false;
        paused = false;

        expected_value = 50;
        result = malbolge_vcpu_address_value(vcpu, expected_address, address_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }
        value_query_hit = false;

        // Resume
        result = malbolge_vcpu_run(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);

        // BP
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return bp_hit; }));
        }
        bp_hit = false;
        paused = false;

        expected_value = 80;
        result = malbolge_vcpu_address_value(vcpu, expected_address, address_value_cb);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return value_query_hit; }));
        }
        value_query_hit = false;

        // Resume
        result = malbolge_vcpu_run(vcpu);
        BOOST_CHECK_EQUAL(result, MALBOLGE_ERR_SUCCESS);
        {
            auto lock = std::unique_lock{mtx};
            BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return waiting; }));
        }

        // Stop
        malbolge_free_vcpu(vcpu);
    }

    {
        auto lock = std::unique_lock{mtx};
        BOOST_CHECK(cv.wait_for(lock, 100ms, [&]() { return stopped; }));
    }
    BOOST_CHECK_EQUAL(output_str, "a");
    BOOST_CHECK(expected_states.empty());
}

BOOST_AUTO_TEST_SUITE_END()
