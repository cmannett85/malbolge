/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#include "malbolge/math/ternary.hpp"

using namespace malbolge;

namespace
{
constexpr auto op_cipher = std::array{
    std::array<std::uint8_t, math::trit::base>{1u, 1u, 2u},
    std::array<std::uint8_t, math::trit::base>{0u, 0u, 2u},
    std::array<std::uint8_t, math::trit::base>{0u, 2u, 1u},
};
}

math::ternary& math::ternary::rotate(std::size_t i) noexcept
{
    v_ = to_tritset().rotate(i).to_base10();
    return *this;
}

math::ternary math::ternary::op(const math::ternary& other) const noexcept
{
    const auto a = to_tritset();
    const auto b = other.to_tritset();

    auto result = tritset_type{};
    for (auto i = 0u; i < tritset_type::width; ++i) {
        result.set(i, op_cipher[a[i]][b[i]]);
    }

    return result.to_base10();
}

std::ostream& std::operator<<(std::ostream& stream,
                              const std::optional<malbolge::math::ternary>& t)
{
    if (!t) {
        return stream << "{}";
    }
    return stream << *t;
}
