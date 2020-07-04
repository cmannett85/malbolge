/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"
#include "malbolge/utility/raii.hpp"
#include "malbolge/exception.hpp"
#include "malbolge/logging.hpp"

using namespace malbolge;
using namespace std::string_literals;

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

std::future<void> virtual_cpu::run(std::istream& istr, std::ostream& ostr)
{
    if (*state_ != execution_state::READY) {
        throw execution_exception{"vCPU is not in a ready state", 0};
    }
    *state_ = execution_state::RUNNING;

    BOOST_LOG_SEV(logging::source::get(), logging::DEBUG)
        << "Starting program";

    auto promise = std::promise<void>{};
    auto fut = promise.get_future();

    thread_ = std::thread{[vmem = std::move(vmem_),
                           state = state_,
                           p = std::move(promise),
                           &istr,
                           &ostr]() mutable {
        BOOST_LOG_SEV(logging::source::get(), logging::VERBOSE_DEBUG)
            << "Program thread started";

        auto a = math::ternary{};   // Accumulator register
        auto c = vmem.begin();     // Code pointer
        auto d = vmem.begin();     // Data pointer

        auto exception = false;
        auto stopped_setter = utility::raii{[&]() {
            BOOST_LOG_SEV(logging::source::get(), logging::VERBOSE_DEBUG)
                << "Program thread exiting";

            *state = execution_state::STOPPED;
            if (!exception) {
                p.set_value();
            }
        }};

        // Loop forever, but increment the pointers on each iteration
        auto step = std::size_t{0};
        for (; true; ++c, ++d, ++step) {
            if (*state == execution_state::STOPPED) {
                return;
            }

            try {
                // Pre-cipher the instruction
                auto instr = pre_cipher_instruction(*c, c - vmem.begin());
                if (!instr) {
                    throw execution_exception{
                        "Pre-cipher non-whitespace character must be graphical "
                            "ASCII: " + std::to_string(static_cast<int>(*c)),
                        step
                    };
                }

                BOOST_LOG_SEV(logging::source::get(), logging::VERBOSE_DEBUG)
                    << "Step: " << step << ", pre-cipher instr: "
                    << static_cast<int>(*instr);

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
                    auto input = char{};
                    if (!istr.get(input)) {
                        a = math::ternary::max;
                    } else {
                        a = input;
                    }

                    break;
                }
                case cpu_instruction::write:
                    if (a != math::ternary::max) {
                        ostr << static_cast<char>(a);
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

                BOOST_LOG_SEV(logging::source::get(), logging::VERBOSE_DEBUG)
                    << "\tPost-op regs - a: " << a
                    << ", c[" << std::distance(vmem.begin(), c) << "]: " << *c
                    << ", d[" << std::distance(vmem.begin(), d) << "]: " << *d;
            } catch (std::exception& e) {
                auto e_ptr = std::make_exception_ptr(std::move(e));
                p.set_exception(std::move(e_ptr));
                exception = true;
                return;
            }
        }
    }};

    return fut;
}

void virtual_cpu::stop()
{
    BOOST_LOG_SEV(logging::source::get(), logging::DEBUG)
        << "Early exit requested";
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
