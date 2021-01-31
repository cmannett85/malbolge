/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/loader.hpp"
#include "malbolge/version.hpp"
#include "malbolge/utility/argument_parser.hpp"
#include "malbolge/debugger/script_parser.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/core/ignore_unused.hpp>

#include <iostream>
#include <optional>

using namespace malbolge;
using namespace std::string_literals;

namespace
{
constexpr auto dbgr_colour = log::colour::BLUE;

[[nodiscard]]
virtual_memory load_program(argument_parser& parser)
{
    auto mode = load_normalised_mode::AUTO;
    if (parser.force_non_normalised()) {
        mode = load_normalised_mode::OFF;
    }

    auto& program = parser.program();
    if (program.source == argument_parser::program_source::DISK) {
        // Load the file off disk
        return load(std::filesystem::path{std::move(program.data)}, mode);
    } else if (program.source == argument_parser::program_source::STRING) {
        // Load from passed in string data
        return load(std::move(program.data), mode);
    } else {
        // Load from stdin
        return load_from_cin(mode);
    }
}

void output_handler(char c)
{
    std::cout << c << std::flush;
}

void input_handler(virtual_cpu& vcpu,
                   boost::asio::posix::stream_descriptor& cin_stream,
                   std::string& buf,
                   boost::system::error_code ec,
                   std::size_t bytes_read)
{
    boost::ignore_unused(bytes_read);

    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            log::print(log::ERROR, "cin read failure: ", ec.message());
        }
        return;
    }

    vcpu.add_input(std::move(buf));

    boost::asio::async_read_until(
        cin_stream,
        boost::asio::dynamic_string_buffer{buf, math::ternary::max},
        '\n',
        [&](auto ec, auto bytes_read) {
            input_handler(vcpu, cin_stream, buf, ec, bytes_read);
    });
}

void run_script_runner(const std::filesystem::path& path, virtual_memory vmem)
{
    const auto seq = debugger::script::parse(path);
    auto runner = debugger::script::script_runner{};

    runner.register_for_output_signal([](auto c) { output_handler(c); });
    runner.register_for_address_value_signal([](auto fn, auto value) {
        log::basic_print(std::clog, dbgr_colour,
                         "[DBGR]: ", fn, " = ", value);
    });
    runner.register_for_register_value_signal([](auto fn, auto address, auto value) {
        log::basic_print(std::clog, dbgr_colour,
                         "[DBGR]: ", fn, " = {", address, ", ", value, "}");
    });

    runner.run(std::move(vmem), seq);
}

void run_program(virtual_memory vmem)
{
    auto ctx = boost::asio::io_context{};
    auto worker_guard = boost::asio::executor_work_guard{ctx.get_executor()};
    auto vcpu = std::make_unique<virtual_cpu>(std::move(vmem));

    vcpu->register_for_output_signal([](auto c) { output_handler(c); });
    vcpu->register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            // Rethrow the exception from the caller's thread
            boost::asio::post(ctx, [eptr]() {
                std::rethrow_exception(eptr);
            });
            return;
        }

        if (state == virtual_cpu::execution_state::STOPPED) {
            boost::asio::post(ctx, [&]() {
                worker_guard.reset();
                ctx.stop();
            });
        }
    });

    // Work for the future: Create an istream wrapper than exposes an
    // AsyncReadStream interface - the following is not cross-platform
    auto cin_stream = boost::asio::posix::stream_descriptor{ctx, ::dup(STDIN_FILENO)};
    auto buf = ""s;
    boost::asio::async_read_until(
        cin_stream,
        boost::asio::dynamic_string_buffer{buf, math::ternary::max},
        '\n',
        [&](auto ec, auto bytes_read) {
            input_handler(*vcpu, cin_stream, buf, ec, bytes_read);
    });

    vcpu->run();
    ctx.run();
}

void run(argument_parser& parser, virtual_memory vmem)
{
    auto script_path = parser.debugger_script();
    if (script_path) {
        run_script_runner(*script_path, std::move(vmem));
    } else {
        run_program(std::move(vmem));
    }
}
}

int main(int argc, char* argv[])
{
    try {
        auto arg_parser = argument_parser{argc, argv};
        if (arg_parser.help()) {
            std::cout << arg_parser << std::endl;
            return EXIT_SUCCESS;
        }

        if (arg_parser.version()) {
            std::cout << "Malbolge Virtual Machine v"
                      << version_string
                      << "\nCopyright Cam Mannett 2020" << std::endl;
            return EXIT_SUCCESS;
        }

        log::set_log_level(arg_parser.log_level());

        auto vmem = load_program(arg_parser);
        run(arg_parser, std::move(vmem));
    } catch (system_exception& e) {
        log::print(log::ERROR, e.what());
        return e.code().value();
    } catch (std::system_error& e) {
        log::print(log::ERROR, e.what());
        return e.code().value();
    } catch (std::exception& e) {
        log::print(log::ERROR, e.what());
        return EXIT_FAILURE;
    } catch (...) {
        log::print(log::ERROR, "Unknown exception");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
