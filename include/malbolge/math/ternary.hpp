/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/tritset.hpp"

/** Top-level namespace for all of malbolge.
 */
namespace malbolge
{
/** Math functions and types.
 */
namespace math
{
/** Ternary unsigned integer type.
 *
 * Malbolge has a single type: a 10 digit ternary (base3) unsigned integer.
 */
class ternary
{
public:
    /** Underlying binary integer type.
     *
     * It is wide enough to hold two lots of the maximum value a 3^10 ternary
     * type can represent, this is to allow the summation (and subsequent
     * modulo) of two Malbolge ternary values.
     */
    using underlying_type = std::uint32_t;

    /** Tritset storage type.
     */
    using tritset_type = tritset<10, std::uint32_t>;

    /** Maximum value representable by this type.
     */
    static constexpr auto max = tritset_type::max;

    /** Constructor.
     *
     * If @a value exceeds max, then it is wrapped.
     * @param value Value to initialise with
     */
    constexpr ternary(underlying_type value = 0) :
        v_{value % (max+1)}
    {}

    /** Tritset constructor.
     *
     * This implicit conversion constructor allows for trit literals to be used
     * when instantiating a ternery, as long as @a N does not exceed 10.
     * @tparam N Number of digits in tritset
     * @tparam T Tritset underlying integer storage type
     * @param value Tritset instance
     */
    template <std::size_t N, typename T>
    constexpr ternary(const tritset<N, T>& value) :
        ternary{value.to_base10()}
    {
        static_assert(N <= tritset_type::width,
                      "Tritset too wide for a Malbolge ternary");
    }

    /** Copy constructor.
     *
     * @param other Instance to copy
     */
    ternary(const ternary& other) = default;

    /** Assignment operator.
     *
     * @param other Instance to copy
     * @return A reference to this
     */
    ternary& operator=(const ternary& other) = default;

    /** Comparison operator.
     *
     * @param other Instance to compare against
     * @return Ordering
     */
    auto operator<=>(const ternary& other) const = default;

    /** Explicit conversion operator for the decimal value.
     *
     * @tparam T Type to convert to
     * @return Decimal value converted to T
     */
    template <typename T>
    explicit operator T() const
    {
        return static_cast<T>(v_);
    }

    /** Return a tritset equivalent to this value.
     *
     * @return Tritset
     */
    tritset_type to_tritset() const
    {
        return tritset_type{v_};
    }

    /** Addition operator.
     *
     * @param other Instance to add
     * @return A copy of this summed with @a other
     */
    ternary operator+(const ternary& other) const
    {
        return v_ + other.v_;
    }

    /** Addition assignment operator.
     *
     * @param other Instance to add
     * @return A reference to this
     */
    ternary& operator+=(const ternary& other)
    {
        return *this = *this + other;
    }

    /** Subtraction operator.
     *
     * @param other Instance to subtract
     * @return A copy of @a other subtracted from this
     */
    ternary operator-(const ternary& other) const
    {
        return other.v_ > v_ ? max - (other.v_ - v_) : v_ - other.v_;
    }

    /** Subtraction assignment operator.
     *
     * @param other Instance to subtract
     * @return A reference to this
     */
    ternary& operator-=(const ternary& other)
    {
        return *this = *this - other;
    }

    /** Modulo operator.
     *
     * @param other Instance to calculate the remainder of
     * @return The remainder of *this divided by @a other
     */
    ternary operator%(const ternary& other) const
    {
        return v_ % other.v_;
    }

    /** Modulo assignment operator.
     *
     * @param other Instance to calculate the remainder of
     * @return A reference to this
     */
    ternary& operator%=(const ternary& other)
    {
        return *this = *this % other;
    }

    /** Rotates the trits to the right (i.e. least-significant).
     *
     * 
     * @param i Number of positions to rotate, modulo-ed to width before use
     * @return A reference to this
     */
    ternary& rotate(std::size_t i = 1);

    /** @em The operation.
     *
     * Each trit is used as input to a 2D LUT, which is mapped into the output:
     * <table>
     * <tr>
     *    <td colspan="2" rowspan="2"></td>
     *    <th colspan="3" style="text-align:center">a</td>
     *  </tr>
     *  <tr>
     *    <th>0</td>
     *    <th>1</td>
     *    <th>2</td>
     *  </tr>
     *  <tr>
     *    <th style="text-align:center" rowspan="3">b</td>
     *    <th>0</td>
     *    <td>1</td>
     *    <td>0</td>
     *    <td>0</td>
     *  </tr>
     *  <tr>
     *    <th>1</td>
     *    <td>1</td>
     *    <td>0</td>
     *    <td>2</td>
     *  </tr>
     *  <tr>
     *    <th>2</td>
     *    <td>2</td>
     *    <td>2</td>
     *    <td>1</td>
     *  </tr>
     *  </table>
     *
     * @param other Instance to operate against
     * @return Operation result
     */
    ternary op(const ternary& other) const;

private:
    underlying_type v_;
};

/** Textual streaming operator.
 *
 * @param stream Output stream
 * @param t Value to write
 * @return @a stream
 */
inline std::ostream& operator<<(std::ostream& stream, const ternary& t)
{
    return stream << t.to_tritset();
}
}
}