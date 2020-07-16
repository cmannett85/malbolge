/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/cpu_instruction.hpp"

using namespace malbolge;

namespace
{
constexpr auto pre_cipher = R"(+b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XTcA"lI)"
                            R"(.v%{gJh4G\-=O@5`_3i<?Z';FNQuY]szf$!BS/|t:Pn6^Ha)";
constexpr auto post_cipher = R"(5z]&gqtyfr$(we4{WP)H-Zn,[%\3dL+Q;>U!pJS72FhOA1C)"
                             R"(B6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G"i@)";
}

char cipher::pre(std::size_t index)
{
    return pre_cipher[index];
}

char cipher::post(std::size_t index)
{
    return post_cipher[index];
}
