/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/normalise.hpp"

using namespace malbolge;

std::string malbolge::normalise_source(const std::string& source)
{
    using std::begin;
    using std::end;

    auto output = std::string{};
    normalise_source(begin(source),
                     end(source),
                     std::back_inserter(output));

    return output;
}

std::string malbolge::denormalise_source(const std::string& source)
{
    using std::begin;
    using std::end;

    auto output = std::string{};
    denormalise_source(begin(source),
                       end(source),
                       std::back_inserter(output));

    return output;
}
