/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/virtual_cpu.hpp"
#include "malbolge/cpu_instruction.hpp"

#include <iostream>
#include <charconv>

using namespace malbolge;
using namespace std::string_literals;

void virtual_cpu::run()
{
    auto a = math::ternary{};   // Accumulator register
    auto c = vmem_.begin();     // Code pointer
    auto d = vmem_.begin();     // Data pointer

    // Loop forever, but increment the pointers on each iteration
    for (; true; ++c, ++d) {
        try {
            // Pre-cipher the instruction
            auto instr = pre_cipher_instruction(*c, c - vmem_.begin());
            switch (instr) {
            case cpu_instruction::set_data_ptr:
                d = vmem_.begin() + static_cast<std::size_t>(*d);
                break;
            case cpu_instruction::set_code_ptr:
                c = vmem_.begin() + static_cast<std::size_t>(*d);
                break;
            case cpu_instruction::rotate:
                d->rotate();
                break;
            case cpu_instruction::op:
                *d = d->op(a);
                break;
            case cpu_instruction::read:
            {
                auto input = char{};
                if (!std::cin.get(input)) {
                    a = math::ternary::max;
                } else if (input == '\n') {
                    a = 10;
                } else {
                    a = input;
                }

                break;
            }
            case cpu_instruction::write:
                if (a == 10) {
                    std::cout << std::endl;
                } else {
                    std::cout << static_cast<char>(a);
                }
                break;
            case cpu_instruction::stop:
                return;
            default:
                // Nop
                break;
            }

            // Post-cipher the instruction
            *c = post_cipher_instruction(instr);
        } catch (std::exception&) {
            return;
        }
    }
}
