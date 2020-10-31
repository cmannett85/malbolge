/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"
#include "malbolge/utility/raii.hpp"
#include "malbolge/utility/stream_lock_guard.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/log.hpp"

using namespace malbolge;
using namespace debugger;
using namespace std::string_literals;
using namespace std::chrono_literals;

virtual_cpu::virtual_cpu(virtual_memory vmem) :
    state_{std::make_shared<decltype(state_)::element_type>(execution_state::READY)},
    vmem_(std::move(vmem))
{}

virtual_cpu::~virtual_cpu()
{
    *state_ = execution_state::STOPPED;
    debugger_.reset();

    if (thread_.joinable()) {
        thread_.join();
    }
}

virtual_cpu& virtual_cpu::operator=(virtual_cpu&& other)
{
    thread_ = std::move(other.thread_);
    state_ = std::move(other.state_);
    vmem_ = std::move(other.vmem_);
    debugger_ = std::move(other.debugger_);

    return *this;
}

std::future<void> virtual_cpu::run(std::istream& istr,
                                   std::ostream& ostr,
                                   utility::mutex_wrapper mtx)
{
    basic_run_check(istr, ostr, mtx);

    log::print(log::DEBUG, "Starting program (future-based)");

    auto promise = std::promise<void>{};
    auto fut = promise.get_future();

    thread_ = std::thread{[vmem = std::move(vmem_),
                           state = state_,
                           p = std::move(promise),
                           &istr,
                           &ostr,
                           mtx = std::move(mtx),
                           debugger = debugger_]() mutable {
        log::print(log::VERBOSE_DEBUG, "Program thread started");

        auto exception = false;
        auto stopped_setter = utility::raii{[&]() {
            *state = execution_state::STOPPED;
            if (!exception) {
                p.set_value();
            }

#ifdef EMSCRIPTEN
            ostr << std::endl;
#endif
            log::print(log::DEBUG, "Program thread exiting");
            if (debugger) {
                debugger->running_cb(client_control::execution_state::NOT_RUNNING);
            }
        }};

        try {
            vcpu_loop(vmem,
                      *state,
                      {},
                      istr,
                      ostr,
                      std::move(mtx),
                      debugger);
        } catch (std::exception& e) {
            auto e_ptr = std::make_exception_ptr(std::move(e));
            p.set_exception(std::move(e_ptr));
            exception = true;
            return;
        }
    }};

    return fut;
}

void virtual_cpu::run(std::function<void (std::exception_ptr)> stopped,
                      std::function<void ()> waiting_for_input,
                      std::istream& istr,
                      std::ostream& ostr,
                      utility::mutex_wrapper mtx)
{
    basic_run_check(istr, ostr, mtx);

    log::print(log::DEBUG, "Starting program (callback-based)");

    thread_ = std::thread{[vmem = std::move(vmem_),
                           state = state_,
                           stopped_cb = std::move(stopped),
                           waiting_cb = std::move(waiting_for_input),
                           &istr,
                           &ostr,
                           mtx = std::move(mtx),
                           debugger = debugger_]() mutable {
        log::print(log::VERBOSE_DEBUG, "Program thread started");

        auto exception = std::exception_ptr{};
        auto stopped_setter = utility::raii{[&]() {
            *state = execution_state::STOPPED;
            stopped_cb(exception);

#ifdef EMSCRIPTEN
            ostr << std::endl;
#endif
            log::print(log::DEBUG, "Program thread exiting");
            if (debugger) {
                debugger->running_cb(client_control::execution_state::NOT_RUNNING);
            }
        }};

        try {
            vcpu_loop(vmem,
                      *state,
                      waiting_cb,
                      istr,
                      ostr,
                      std::move(mtx),
                      debugger);
        } catch (std::exception& e) {
            exception = std::current_exception();
            return;
        }
    }};
}

void virtual_cpu::stop()
{
    if (*state_ != execution_state::RUNNING) {
        return;
    }

    log::print(log::DEBUG, "Early exit requested");
    *state_ = execution_state::STOPPED;
}

vcpu_control
virtual_cpu::configure_debugger(running_callback_type running,
                                step_data_callback_type step_data)
{
    if (!running || !step_data) {
        throw basic_exception{"Debugger callbacks must be valid"};
    }

    if (*state_ != execution_state::READY) {
        throw basic_exception{"Cannot configure debugger as program not ready"};
    }

    if (debugger_) {
        throw basic_exception{"Debugger already configured"};
    }

    debugger_ = std::make_shared<debugger_data>();
    debugger_->running_cb = std::move(running);
    debugger_->step_data_cb = std::move(step_data);
    debugger_->gate = utility::gate{[this](bool closed) {
        debugger_->running_cb(closed ?
            client_control::execution_state::PAUSED :
            client_control::execution_state::RUNNING);
    }};

    auto controller = vcpu_control{};
    controller.pause  = [this]() {
        debugger_->gate.close();
    };
    controller.step   = [this]() { debugger_->gate.open(1); };
    controller.resume = [this]() { debugger_->gate.open(); };

    controller.address_value = [this](auto address) {
        return debugger_->address_value(address);
    };
    controller.register_value = [this](auto reg) {
        return debugger_->register_value(reg);
    };

    return controller;
}

void virtual_cpu::basic_run_check(std::istream& istr,
                                  std::ostream& ostr,
                                  utility::mutex_wrapper mtx)
{
    if (*state_ != execution_state::READY) {
        throw execution_exception{"vCPU is not in a ready state", 0};
    }

    if (!mtx && (istr.rdbuf() != std::cin.rdbuf() ||
                 ostr.rdbuf() != std::cout.rdbuf())){
        throw execution_exception{
            "Non-stdio I/O streams provided but no mutex",
            0
        };
    }

    *state_ = execution_state::RUNNING;
}

void virtual_cpu::vcpu_loop(virtual_memory& vmem,
                            std::atomic<virtual_cpu::execution_state>& state,
                            std::function<void ()> waiting_for_input,
                            std::istream& istr,
                            std::ostream& ostr,
                            utility::mutex_wrapper mtx,
                            std::shared_ptr<debugger_data> debugger)
{
    auto a = math::ternary{};   // Accumulator register
    auto c = vmem.begin();      // Code pointer
    auto d = vmem.begin();      // Data pointer

    auto reading_stream = false;

    // Minimise branching by using no-op functions when there's no debugger in
    // use
    auto pause_check = std::function<void ()>{[]() {}};
    auto step_check = std::function<void (virtual_memory::iterator,
                                          vcpu_register::id)>{[](auto, auto) {}};
    if (debugger) {
        pause_check = [&]() {
            debugger->gate();
        };
        step_check = [&](virtual_memory::iterator reg_it,
                         vcpu_register::id reg) {
            const auto index = static_cast<math::ternary::underlying_type>(
                std::distance(vmem.begin(), reg_it)
            );
            const auto stop = debugger->step_data_cb(index, reg);
            if (stop) {
                debugger->gate.close();
                debugger->gate();
            }
        };

        // Populate the memory access functions
        debugger->address_value = [&](math::ternary address) {
            return vmem[static_cast<math::ternary::underlying_type>(address)];
        };
        debugger->register_value = [&](vcpu_register::id reg) {
            switch (reg) {
            case vcpu_register::A:
                return vcpu_register::data{a};
            case vcpu_register::C:
            {
                const auto address = static_cast<math::ternary::underlying_type>(
                                        std::distance(vmem.begin(), c));
                return vcpu_register::data{address, vmem[address]};
            }
            case vcpu_register::D:
            {
                const auto address = static_cast<math::ternary::underlying_type>(
                                        std::distance(vmem.begin(), d));
                return vcpu_register::data{address, vmem[address]};
            }
            default:
                throw basic_exception{
                    "Unhandled register query: " + std::to_string(reg)
                };
            }
        };
    }

    // Loop forever, but increment the pointers on each iteration
    auto step = std::size_t{0};
    for (; true; ++c, ++d, ++step) {
        if (state == virtual_cpu::execution_state::STOPPED) {
            return;
        }

        pause_check();
        step_check(c, vcpu_register::C);

        // Pre-cipher the instruction
        auto instr = pre_cipher_instruction(*c, c - vmem.begin());
        if (!instr) {
            throw execution_exception{
                "Pre-cipher non-whitespace character must be graphical "
                    "ASCII: " + std::to_string(static_cast<int>(*c)),
                step
            };
        }

        log::print(log::VERBOSE_DEBUG,
                   "Step: ", step, ", pre-cipher instr: ",
                   static_cast<int>(*instr));

        switch (*instr) {
        case cpu_instruction::set_data_ptr:
            step_check(d, vcpu_register::D);
            d = vmem.begin() + static_cast<std::size_t>(*d);
            break;
        case cpu_instruction::set_code_ptr:
            step_check(d, vcpu_register::D);
            c = vmem.begin() + static_cast<std::size_t>(*d);
            break;
        case cpu_instruction::rotate:
            step_check(d, vcpu_register::D);
            a = d->rotate();
            break;
        case cpu_instruction::op:
            step_check(d, vcpu_register::D);
            a = *d = a.op(*d);
            break;
        case cpu_instruction::read:
        {
            // We can't block on waiting for input because it prevents the
            // thread from exiting, so we poll the input stream, checking for
            // a stop state in between each cycle
            // Issue #97
            auto waiting_called = false;
            while (true) {
                if (state == virtual_cpu::execution_state::STOPPED) {
                    return;
                }

                {
                    auto guard = stream_lock_guard{mtx, istr};
                    if (istr.peek() < 0) {
                        // No data
                        if (!reading_stream) {
                            // Only call the waiting callback once per read
                            // operation
                            if (!waiting_called && waiting_for_input) {
                                waiting_for_input();
                                waiting_called = true;
                            }

                            // Clear the fail bits
                            istr.clear();
                        } else {
                            // We have finished reading the data
                            reading_stream = false;
                            a = math::ternary::max;
                            istr.clear();
                            break;
                        }
                    } else {
                        // We have found data to read
                        reading_stream = true;
                        a = istr.get();
                        break;
                    }
                }

                std::this_thread::sleep_for(25ms);
            }
            break;
        }
        case cpu_instruction::write:
            if (a != math::ternary::max) {
                auto guard = stream_lock_guard{mtx, ostr};
#ifdef EMSCRIPTEN
                // Emscripten cannot flush output without a newline, so
                // we need to force with std::endl
                if (static_cast<char>(a) == '\n') {
                    ostr << std::endl;
                } else {
                    ostr << static_cast<char>(a);
                }
#else
                ostr << static_cast<char>(a);
#endif
            }
            break;
        case cpu_instruction::stop:
            return;
        default:
            // Nop
            break;
        }

        // Post-cipher the instruction
        auto pc = post_cipher_instruction(*c);
        if (!pc) {
            throw execution_exception{
                "Post-cipher non-whitespace character must be graphical "
                    "ASCII: " + std::to_string(static_cast<int>(*c)),
                step
            };
        }
        *c = *pc;

        log::print(log::VERBOSE_DEBUG,
                   "\tPost-op regs - a: ", a,
                   ", c[", std::distance(vmem.begin(), c), "]: ", *c,
                   ", d[", std::distance(vmem.begin(), d), "]: ", *d);
    }
}

std::ostream& malbolge::operator<<(std::ostream& stream,
                                   virtual_cpu::execution_state state)
{
    static_assert(static_cast<int>(virtual_cpu::execution_state::NUM_STATES) == 3,
                  "Number of execution states have change, update operator<<");

    switch (state) {
    case virtual_cpu::execution_state::READY:
        return stream << "READY";
    case virtual_cpu::execution_state::RUNNING:
        return stream << "RUNNING";
    case virtual_cpu::execution_state::STOPPED:
        return stream << "STOPPED";
    default:
        return stream << "Unknown";
    }
}
