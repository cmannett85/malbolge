/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/debugger/script_runner.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/log.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/post.hpp>

using namespace malbolge;
using namespace debugger;

namespace
{
void validate_sequence(const script::functions::sequence& fn_seq)
{
    // Check that:
    //  - There is one, and only one, run function
    //  - A step or resume function do not appear before a run
    //  - If there are any, then at least one add_breakpoint appears before a
    //    rund

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
    if (std::any_of(run_it+1, name_seq.end(),
                    [](auto name) { return name == "run"; })) {
        throw basic_exception{"There can only be one run function"};
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
}
}

std::ostream& script::operator<<(std::ostream& stream,
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

std::ostream& script::operator<<(std::ostream& stream,
                                 const functions::sequence& seq)
{
    for (const auto& fn_var : seq) {
        stream << fn_var << std::endl;
    }
    return stream;
}

void script::script_runner::run(virtual_memory vmem,
                                const functions::sequence& fn_seq)
{
    validate_sequence(fn_seq);

    auto ctx = boost::asio::io_context{};
    auto work_guard = boost::asio::executor_work_guard{ctx.get_executor()};
    auto run_timer = boost::asio::steady_timer{ctx};

    // Instantiate the vCPU and hook up the signals
    auto vcpu = std::make_unique<virtual_cpu>(std::move(vmem));

    auto seq_it = fn_seq.begin();
    auto run_seq = [&]() {
        auto exit = false;
        for (; seq_it != fn_seq.end(); ++seq_it) {
            if (exit) {
                return;
            }

            utility::visit(
                *seq_it,
                [&](const functions::add_breakpoint& fn) {
                    vcpu->add_breakpoint(fn.value<MAL_STR(address)>(),
                                         fn.value<MAL_STR(ignore_count)>());
                },
                [&](const functions::remove_breakpoint& fn) {
                    vcpu->remove_breakpoint(fn.value<MAL_STR(address)>());
                },
                [&](const functions::run& fn) {
                    const auto runtime = fn.value<MAL_STR(max_runtime_ms)>();
                    if (runtime) {
                        run_timer.expires_after(std::chrono::milliseconds{runtime});
                        run_timer.async_wait([&](auto ec) {
                            if (!ec) {
                                log::print(log::DEBUG, "Script runtime timeout reached");
                                vcpu.reset();
                            }
                        });
                    }

                    vcpu->run();
                    exit = true;
                },
                [&](const functions::address_value& fn) {
                    const auto address = fn.value<MAL_STR(address)>();
                    vcpu->address_value(address, [&](auto, auto value) {
                        address_sig_(fn, value);
                    });
                },
                [&](const functions::register_value& fn) {
                    vcpu->register_value(fn.value<MAL_STR(reg)>(),
                                         [&](auto, auto addr, auto value) {
                        reg_sig_(fn, std::move(addr), value);
                    });
                },
                [&](const functions::step&) {
                    vcpu->step();
                },
                [&](const functions::resume&) {
                    vcpu->run();
                    exit = true;
                },
                [&](const functions::on_input& fn) {
                    vcpu->add_input(fn.value<MAL_STR(data)>());
                }
            );
        }
    };

    vcpu->register_for_output_signal([&](auto c) {
        output_sig_(c);
    });
    vcpu->register_for_breakpoint_hit_signal([&](auto) {
        // We've hit a breakpoint so cancel the max runtime timer.  This is a
        // no-op if the timer is not running
        run_timer.cancel();

        // Continue the function sequence.  We post here because this slot is
        // called from the vCPU's worker thread
        boost::asio::post(ctx, [&]() { run_seq(); });
    });
    vcpu->register_for_state_signal([&](auto state, auto eptr) {
        if (eptr) {
            // Rethrow the exception from the caller's thread
            boost::asio::post(ctx, [eptr]() {
                std::rethrow_exception(eptr);
            });
            return;
        }

        if (state == virtual_cpu::execution_state::STOPPED) {
            // If the vCPU has stopped, we need to stop too.  We do not call
            // ctx.stop() as all queued jobs need processing, specifically
            // errors
            run_timer.cancel();
            work_guard.reset();
        }
    });

    boost::asio::post(ctx, [&]() { run_seq(); });
    ctx.run();
}
