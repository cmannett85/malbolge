/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_runner.hpp"
#include "malbolge/utility/visit.hpp"
#include "malbolge/virtual_cpu.hpp"
#include "malbolge/log.hpp"

#include <sstream>
#include <deque>
#include <algorithm>
#include <vector>

using namespace malbolge;
using namespace debugger;
using namespace std::string_view_literals;

namespace
{
void validate_sequence(const script::functions::sequence& fn_seq)
{
    // Check that:
    //  - There is one, and only one, run function
    //  - A step or resume function do not appear before a run
    //  - If there are any, then at least one add_breakpoint appears before a
    //    run
    //  - Nothing appears after a stop

    // Convert to a string_view list first as it's easier to use 'normal'
    // algorithms with
    auto name_seq = std::vector<std::string_view>{};
    name_seq.reserve(fn_seq.size());

    for (const auto& var_fn : fn_seq) {
        utility::visit(
            var_fn,
            [&]<typename T>(const T&) {
                name_seq.push_back(T::name());
            }
        );
    }

    const auto run_it = std::find(name_seq.begin(), name_seq.end(), "run");
    if (run_it == name_seq.end()) {
        throw basic_exception{"There must be at least one run function"};
    }

    for (auto it = name_seq.begin(); it != run_it; ++it) {
        if (*it == "step" || *it == "resume") {
            throw basic_exception{"Step or resume functions cannot appear "
                                  "before a run"};
        }
    }

    auto first_bp_it = std::find(name_seq.begin(), name_seq.end(),
                                 "add_breakpoint");
    if (first_bp_it != name_seq.end() && first_bp_it > run_it) {
        throw basic_exception{"If there are any add_breakpoint functions, at "
                              "least one must appear before a run"};
    }

    auto stop_it = std::find(name_seq.begin(), name_seq.end(), "stop");
    if (stop_it != name_seq.end() && stop_it != (name_seq.end()-1)) {
        throw basic_exception{"If a stop is present, it must be last"};
    }
}
}

std::ostream&
debugger::script::operator<<(std::ostream& stream,
                             const functions::function_variant& fn)
{
    utility::visit(
        fn,
        [&](const auto& fn) {
            stream << fn;
        }
    );
    return stream;
}

std::ostream&
debugger::script::operator<<(std::ostream& stream,
                             const functions::sequence& seq)
{
    for (const auto& fn_var : seq) {
        stream << fn_var << std::endl;
    }
    return stream;
}

void debugger::script::run(const functions::sequence& fn_seq,
                           virtual_cpu& vcpu,
                           std::ostream& dstr,
                           std::ostream& ostr)
{
    validate_sequence(fn_seq);

    auto cc = client_control{vcpu};
    auto gate = utility::gate{};

    // For blocking until program has exited
    auto exit = std::atomic_bool{false};
    auto exit_gate = utility::gate{};
    exit_gate.close();

    // Input stream.  We have to delegate the input stream writing into a
    // separate thread (instead of doing it in waiting_input_cb) because
    // otherwise input_mtx will deadlock.  This should be solved with the
    // outcome of Issue #145.
    auto input_mtx = std::mutex{};
    auto input_queue = std::deque<std::string_view>{};
    auto input_str = std::stringstream{};
    auto input_gate = utility::gate{};
    auto input_thread = std::thread{[&]() {
        input_gate.close();
        while (!exit) {
            input_gate();
            if (exit) {
                return;
            }

            auto lock = std::lock_guard{input_mtx};
            if (input_queue.empty()) {
                gate.close();
                continue;
            }

            input_str << input_queue.front() << std::endl;
            input_queue.pop_front();
        }
    }};

    // Run timer
    auto run_timer_mtx = std::mutex{};
    auto run_timer_cv = std::condition_variable{};
    auto run_timer_bp_hit = false;
    auto run_timer_thread = std::thread{};

    // Stop and wait handlers for the vCPU
    auto ex_ptr = std::exception_ptr{};
    auto stopped_cb = [&](std::exception_ptr e) {
        ex_ptr = std::move(e);
        exit = true;

        {
            auto lock = std::lock_guard{run_timer_mtx};
            run_timer_cv.notify_one();
        }

        gate.open();
        input_gate.open();
        exit_gate.open();
    };
    auto waiting_input_cb = [&]() {
        input_gate.open();
    };
    auto gate_check = [&]() {
        gate();
        return exit.load();
    };

    for (const auto& var_fn : fn_seq) {
        if (exit) {
            break;
        }

        utility::visit(
            var_fn,
            [&](const functions::add_breakpoint& fn) {
                if (gate_check()) {
                    return;
                }

                auto cb = [&](auto, auto) {
                    {
                        auto lock = std::lock_guard{run_timer_mtx};
                        run_timer_bp_hit = true;
                        run_timer_cv.notify_one();
                    }
                    gate.open();
                    return true;
                };

                cc.add_breakpoint(client_control::breakpoint{
                    fn.value<MAL_STR(address)>(),
                    std::move(cb),
                    fn.value<MAL_STR(ignore_count)>()
                });
            },
            [&](const functions::remove_breakpoint& fn) {
                if (gate_check()) {
                    return;
                }

                cc.remove_breakpoint(fn.value<MAL_STR(address)>());
            },
            [&](const functions::run& fn) {
                gate.close();

                const auto run_ms = fn.value<MAL_STR(max_runtime_ms)>();
                if (run_ms) {
                    run_timer_thread = std::thread{[&, run_ms]() {
                        {
                            auto lock = std::unique_lock{run_timer_mtx};
                            run_timer_cv.wait_for(
                                lock,
                                std::chrono::milliseconds{run_ms},
                                [&]() { return run_timer_bp_hit || exit; });
                        }
                        if (!run_timer_bp_hit) {
                            vcpu.stop();
                        }
                    }};
                }

                vcpu.run(std::move(stopped_cb),
                         std::move(waiting_input_cb),
                         input_str,
                         ostr,
                         input_mtx);
            },
            [&](const functions::address_value& fn) {
                if (gate_check()) {
                    return;
                }

                const auto address = fn.value<MAL_STR(address)>();
                const auto value = cc.address_value(address);
                log::basic_print(dstr, "[DBGR]: ", fn, " = ", value);
            },
            [&](const functions::register_value& fn) {
                if (gate_check()) {
                    return;
                }

                const auto reg = fn.value<MAL_STR(reg)>();
                const auto value = cc.register_value(reg);
                log::basic_print(dstr, "[DBGR]: ", fn, " = ", value);
            },
            [&](const functions::step&) {
                if (gate_check()) {
                    return;
                }

                gate.close();
                cc.step([&]() {
                    gate.open();
                });
            },
            [&](const functions::resume&) {
                gate.close();
                cc.resume();
            },
            [&](const functions::stop&) {
                vcpu.stop();
            },
            [&](const functions::on_input& fn) {
                auto lock = std::lock_guard{input_mtx};
                input_queue.push_back(fn.value<MAL_STR(data)>());
            }
        );
    }

    // Do not exit until the program has finished
    exit_gate();
    if (run_timer_thread.joinable()) {
        run_timer_thread.join();
    }
    input_thread.join();

    // If the program throws an exception during execution, then rethrow here
    if (ex_ptr) {
        std::rethrow_exception(ex_ptr);
    }
}
