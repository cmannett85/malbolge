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

namespace
{
constexpr auto cipher_size = graphical_ascii_range.second -
                             graphical_ascii_range.first;
constexpr auto pre_cipher = R"(+,b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XTcA"lI)"
                            R"(.v%{gJh4G\-=O@5`_3i<?Z';FNQuY]szf$!BS/|t:Pn6^Ha)";
constexpr auto post_cipher = R"(5z]&gqtyfr$(we4{WP)H-Zn,[%\3dL+Q;>U!pJS72FhOA1C)"
                             R"(B6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G"i@)";
}

void virtual_cpu::run()
{
    auto a = math::ternary{};   // Accumulator register
    auto c = vmem_.begin();     // Code pointer
    auto d = vmem_.begin();     // Data pointer

    // Loop forever, but increment the pointers on each iteration
    for (; true; ++c, ++d) {
        if (is_graphical_ascii(*c)) {
            // Pre-cipher the instruction
            auto index = (*c - graphical_ascii_range.first + (c - vmem_.begin())) %
                         (cipher_size+1);
            const auto instr = pre_cipher[static_cast<std::size_t>(index)];

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

            // Post-cipher the (possibly) new instruction
            index -= graphical_ascii_range.first;
            *c = post_cipher[static_cast<std::size_t>(index)];
        } else {
            return;
        }
    }
}
