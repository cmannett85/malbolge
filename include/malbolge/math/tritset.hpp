/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/ipow.hpp"
#include "malbolge/traits.hpp"

#include <ostream>
#include <compare>

namespace malbolge
{
namespace math
{
/** Namespace for trit constants.
 */
namespace trit
{
/** Trit number base, always 3.
 */
constexpr auto base = std::uint8_t{3};

/** Number of bits required to represent a trit.
 */
constexpr auto bits_per_trit = std::uint8_t{2};

/** A trait that returns the minimum binary integral type required to represent
 * a ternary type.
 *
 * @tparam N Number of digits
 */
template <std::size_t N>
struct min_width_type
{
private:
    static constexpr auto calculate_type()
    {
        if constexpr ((sizeof(std::uint8_t)*8) >= (N*bits_per_trit)) {
            return std::uint8_t{};
        } else if constexpr ((sizeof(std::uint16_t)*8) >= (N*bits_per_trit)) {
            return std::uint16_t{};
        } else if constexpr ((sizeof(std::uint32_t)*8) >= (N*bits_per_trit)) {
            return std::uint32_t{};
        } else {
            static_assert(traits::always_false_v<decltype(N)>,
                          "Number of trits required exceeds maximum");
        }
    }

public:

    /** Minimum binary integral type that can represent an @a N digit ternary
     * number.
     */
    using type = decltype(calculate_type());
};

/** Helper type for min_width_type.
 *
 * @tparam N Number of digits
 */
template <std::size_t N>
using min_width_type_t = typename min_width_type<N>::type;
}

/** Simple ternary bitset equivalent.
 *
 * Allows trit manipulation of a ternary value.
 * @tparam N Number of trits in set
 * @tparam T Underlying integer storage type, defaults to
 * trit::min_width_type_t<N>
 */
template <std::size_t N, typename T = trit::min_width_type_t<N>>
class tritset
{
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned integral");
    static_assert(N > 0u, "N must be greater than zero");

    static constexpr auto bmask = 0b11;

public:
    /** Underlying storage type.
     */
    using underlying_type = T;

    /** Width (in trits) of this type.
     */
    static constexpr auto width = N;

    /** Maximum value representable by this type.
     */
    static constexpr auto max = static_cast<T>(ipow<std::uint64_t, trit::base, width>() - 1);

    static_assert((sizeof(T)*8) >= (N*trit::bits_per_trit),
                  "T must be wide enough to hold all trits");

    /** Constructor.
     *
     * @a value is modulo-ed with max before processing
     * @param value Decimal value to initialise with
     */
    constexpr explicit tritset(T value = 0) noexcept :
        v_{0}
    {
        auto q = value % (max+1);
        for (auto i = 0u; i < width; ++i) {
            v_ |= ((q % trit::base) & bmask) << (i*trit::bits_per_trit);
            if (q < trit::base) {
                break;
            }
            q /= trit::base;
        }
    }

    /** Convert a string in ternary form to a ternary.
     *
     * The string must be contain only the characters [0-2].
     *
     * For compile-time ternarys in string form, use the trit literal.
     * @param str Input string
     * @exception std::invalid_argument Thrown if str contains invalid
     * characters, or contains more characters than width
     */
    constexpr explicit tritset(std::string_view str) :
        v_{0}
    {
        if (str.size() > width) {
            throw std::invalid_argument{"Too many characters in string"};
        }

        auto i = 0u;
        for (auto it = str.rbegin(); it != str.rend(); ++it) {
            const auto c = *it;
            if (c != '0' && c != '1' && c != '2') {
                throw std::invalid_argument{"Invalid character in string"};
            }
            set(i++, c);
        }
    }

    /** Copy constructor.
     *
     * @param other Instance to copy from
     */
    constexpr tritset(const tritset& other) noexcept = default;

    /** Assignment operator.
     *
     * @param other Instance to copy from
     * @return A reference to this
     */
    constexpr tritset& operator=(const tritset& other) noexcept = default;

    /** Comparison operator.
     *
     * @param other Instance to copy from
     * @return Ordering type
     */
    [[nodiscard]]
    constexpr auto operator<=>(const tritset& other) const noexcept = default;

    /** Return a decimal representation of the current value.
     *
     * @return Decimal
     */
    [[nodiscard]]
    constexpr T to_base10() const noexcept
    {
        auto result = T{0};
        boost::mp11::mp_for_each<boost::mp11::mp_iota_c<width>>([&](auto i) {
            const auto p = ipow<T, trit::base, decltype(i)::value>();
            result += (*this)[i] * p;
        });

        return result;
    }

    /** Trit accessor.
     *
     * @param i Trit index to return
     * @return Trit at index @a i
     */
    [[nodiscard]]
    constexpr std::uint8_t operator[](std::size_t i) const noexcept
    {
        return (v_ >> (i*trit::bits_per_trit)) & bmask;
    }

    /** Sets the trit at @a i with @a value.
     *
     * Only the first two bits of @a value are used.
     * @param i Trit index
     * @param value New value
     * @return A reference to this
     */
    constexpr tritset& set(std::size_t i, std::uint8_t value) noexcept
    {
        const auto bits = (value & bmask) << (i*trit::bits_per_trit);
        v_ |= bits;

        return *this;
    }

    /** Rotates the trits to the right (i.e. least-significant).
     *
     * @param i Number of positions to rotate, modulo-ed to width before use
     * @return A reference to this
     */
    constexpr tritset& rotate(std::size_t i = 0u) noexcept
    {
        i %= width;
        if (i != 0u) {
            const auto mask = ipow<T>(2u, i*trit::bits_per_trit)-1;
            const auto shift =(width*trit::bits_per_trit) - (i*trit::bits_per_trit);
            const auto prefix = (v_ & mask) << shift;

            v_ >>= i*trit::bits_per_trit;
            v_ |= prefix;
        }

        return *this;
    }

private:
    T v_;
};

/** Textual streaming operator.
 *
 * @tparam N Number of trits in set
 * @tparam T Underlying integer storage type
 * @param stream Stream to write to
 * @param t Tritset to write
 * @return @a stream
 */
template <std::size_t N, typename T>
inline std::ostream& operator<<(std::ostream& stream, const tritset<N, T>& t)
{
    stream << "{d:" << std::dec << t.to_base10() << ", t:";

    for (auto i = static_cast<int>(N-1); i >= 0; --i) {
        stream << std::dec << static_cast<int>(t[i]);
    }

    return stream << "}";
}
}

/** Namespace for Malbolge literals.
 */
namespace literals
{
/** Creates a tritset from a literal representation of a ternary value.
 *
 * The most-significant trit is first
 * @code
 * const auto t = 222_trit; // t == 26u
 * @endcode
 *
 * The number of provided digits defines the number of digits in the return
 * type i.e.:
 * @code
 * std::is_same_v<decltype(222_trit), tritset<3>> // true
 * @endcode
 * @tparam Cs Char array
 * @return Tritset instance
 */
template <char... Cs>
constexpr auto operator"" _trit() noexcept
{
    static_assert(((Cs == '0' || Cs == '1' || Cs == '2') && ...),
                  "All digits must be ternary (i.e. [0-3))");

    using chars = boost::mp11::mp_reverse<
                    std::tuple<traits::integral_constant<Cs-'0'>...>>;
    constexpr auto digits = sizeof...(Cs);

    auto t = math::tritset<digits>{};
    boost::mp11::mp_for_each<boost::mp11::mp_iota_c<digits>>([&](auto i) {
        t.set(i, std::tuple_element_t<decltype(i)::value, chars>::value);
    });

    return t;
}
}
}
