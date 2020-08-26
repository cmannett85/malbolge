/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"
#include "malbolge/utility/get_char.hpp"
#include "malbolge/utility/raii.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/log.hpp"

using namespace malbolge;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace
{
void vcpu_loop(virtual_memory& vmem,
               std::atomic<virtual_cpu::execution_state>& state,
               std::function<void ()> waiting_for_input,
               std::istream& istr,
               std::ostream& ostr,
               utility::mutex_wrapper mtx)
{
    auto a = math::ternary{};   // Accumulator register
    auto c = vmem.begin();      // Code pointer
    auto d = vmem.begin();      // Data pointer

    // Loop forever, but increment the pointers on each iteration
    auto step = std::size_t{0};
    for (; true; ++c, ++d, ++step) {
        if (state == virtual_cpu::execution_state::STOPPED) {
            return;
        }

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
            d = vmem.begin() + static_cast<std::size_t>(*d);
            break;
        case cpu_instruction::set_code_ptr:
            c = vmem.begin() + static_cast<std::size_t>(*d);
            break;
        case cpu_instruction::rotate:
            a = d->rotate();
            break;
        case cpu_instruction::op:
            a = *d = a.op(*d);
            break;
        case cpu_instruction::read:
        {
            if (waiting_for_input) {
                waiting_for_input();
            }

            // We can't block on waiting for input because it prevents the
            // thread from exiting, so we poll the input stream, checking for
            // a stop state in between each cycle
            // Issue #97
            while (true) {
                if (state == virtual_cpu::execution_state::STOPPED) {
                    return;
                }

                auto input = char{};
                auto result = utility::get_char_result::NO_DATA;
                {
                    auto guard = std::lock_guard{mtx};
                    result = utility::get_char(istr, input, false);
                }

                if (result == utility::get_char_result::NO_DATA) {
                    std::this_thread::sleep_for(25ms);
                } else if (result == utility::get_char_result::CHAR) {
                    a = input;
                    break;
                } else {
                    a = math::ternary::max;
                    break;
                }
            }
            break;
        }
        case cpu_instruction::write:
            if (a != math::ternary::max) {
#ifdef EMSCRIPTEN
                // Emscripten cannot flush output without a newline, so
                // we need to force with std::endl
                {
                    auto guard = std::lock_guard{mtx};
                    if (static_cast<char>(a) == '\n') {
                        ostr << std::endl;
                    } else {
                        ostr << static_cast<char>(a);
                    }
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
}

virtual_cpu::~virtual_cpu()
{
    *state_ = execution_state::STOPPED;
    if (thread_.joinable()) {
        thread_.join();
    }
}

virtual_cpu& virtual_cpu::operator=(virtual_cpu&& other)
{
    // Stop this vCPU
    *state_ = execution_state::STOPPED;
    if (thread_.joinable()) {
        thread_.join();
    }

    // Assign the new one
    thread_ = std::move(other.thread_);
    state_ = std::move(other.state_);
    vmem_ = std::move(other.vmem_);

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
                           mtx = std::move(mtx)]() mutable {
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
        }};

        try {
            vcpu_loop(vmem, *state, {}, istr, ostr, std::move(mtx));
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
                           mtx = std::move(mtx)]() mutable {
        log::print(log::VERBOSE_DEBUG, "Program thread started");

        auto exception = std::exception_ptr{};
        auto stopped_setter = utility::raii{[&]() {
            *state = execution_state::STOPPED;
            stopped_cb(exception);

#ifdef EMSCRIPTEN
            ostr << std::endl;
#endif
            log::print(log::DEBUG, "Program thread exiting");
        }};

        try {
            vcpu_loop(vmem, *state, waiting_cb, istr, ostr, std::move(mtx));
        } catch (std::exception& e) {
            exception = std::current_exception();
            return;
        }
    }};
}

void virtual_cpu::stop()
{
    log::print(log::DEBUG, "Early exit requested");
    *state_ = execution_state::STOPPED;
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
