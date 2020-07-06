/* Cam Mannett 2020
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/math/ternary.hpp"
#include "malbolge/exception.hpp"

#include <array>
#include <span>
#include <memory>

namespace malbolge
{
/** Represents the virtual machines memory.
 *
 * This class can not be copied, but can be moved.
 */
class virtual_memory
{
    using base = std::unique_ptr<std::array<math::ternary, math::ternary::max+1>>;

public:
    /** Memory 'cell' type.
     */
    using value_type = base::element_type::value_type;

    /** Iterator class.
     *
     * Following the rules of Malbolge's vmem (https://en.wikipedia.org/wiki/Malbolge#Memory),
     * incrementing pointers past the end wraps them back
     * round to zero.  This of course means that an <TT>end()</TT> will never be
     * reached, so loops like this:
     * @code
     * auto vmem = virtual_memory(..);
     * for (auto& t : vmem) {
     *     ...
     * }
     * @endcode
     * Will @em never end!  They need to be explicitly broken out of, but that
     * should only happen when a Malbolge program ends.
     *
     * @note This class cannot be copied, but can be moved
     * @tparam IsConstant True if this is a constant iterator
     */
    template <bool IsConstant>
    class iterator_generic
    {      
    public:
        using difference_type   = virtual_memory::base::element_type::difference_type;  ///< Difference type
        using value_type        = std::conditional_t<IsConstant,
                                                     const virtual_memory::value_type,
                                                     virtual_memory::value_type>;       ///< Value type
        using pointer           = value_type*;                      ///< Pointer to value type
        using reference         = value_type&;                      ///< Reference to value type
        using iterator_category = std::random_access_iterator_tag;  ///< Iterator category

        /** Copy constructor.
         *
         * @param other Instance to copy from
         */
        iterator_generic(const iterator_generic& other)  = default;

        /** Assignment operator.
         *
         * @param other Instance to copy from
         * @return A reference to this
         */
        iterator_generic& operator=(const iterator_generic& other) = default;

        /** Dereference operator.
         *
         * @return A reference to the element the iterator points to
         */
        reference operator*() const
        {
            return *current_;
        }

        /** Member-of operator for value_type.
         *
         * @return Pointer to the element the iterator points to
         */
        pointer operator->() const
        {
            return &(*current_);
        }

        /** Pre-increment operator.
         *
         * If incrementing results in this iterator being equal to
         * <TT>end()</TT>, then the iterator is set to <TT>begin()</TT>.
         * @return A reference to this
         */
        iterator_generic& operator++()
        {
            current_ = current_ == (data_.end()-1) ? data_.begin() : ++current_;
            return *this;
        }

        /** Post-increment operator.
         *
         * If incrementing results in this iterator being equal to
         * <TT>end()</TT>, then the iterator is set to <TT>begin()</TT>.
         * @return A copy of the iterator before the increment
         */
        iterator_generic operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        /** Comparison operator.
         *
         * @param other Instance to compare against
         * @return Ordering
         */
        auto operator<=>(const iterator_generic& other) const
        {
            return current_ <=> other.current_;
        }

        /** Pre-decrement operator.
         *
         * If decrementing results in this iterator being equal to
         * <TT>begin()-1</TT>, then the iterator is set to <TT>end()-1</TT>.
         * @return A reference to this
         */
        iterator_generic& operator--()
        {
            current_ = current_ == data_.begin() ? (data_.end()-1) : --current_;
            return *this;
        }

        /** Post-decrement operator.
         *
         * If decrementing results in this iterator being equal to
         * <TT>begin()-1</TT>, then the iterator is set to <TT>end()-1</TT>.
         * @return A copy of the iterator before the decrement
         */
        iterator_generic operator--(int)
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        /** Addition assignment operator.
         *
         * Incrementing beyond the end of the memory space will cause the
         * pointer to wrap around.
         * @param offset Element offset, may be negative
         * @return A reference to this
         */
        iterator_generic& operator+=(difference_type offset)
        {
            const auto positive = offset >= 0;
            offset = std::abs(offset) % data_.size();
            if (positive) {
                const auto dist_to_end = std::distance(current_, data_.end());
                if (offset > dist_to_end) {
                    offset -= dist_to_end;
                    current_ = data_.begin() + offset;
                } else {
                    current_ += offset;
                }
            } else {
                const auto dist_to_start = std::distance(data_.begin(), current_);
                if (offset > dist_to_start) {
                    offset -= dist_to_start;
                    current_ = data_.end() - offset;
                } else {
                    current_ -= offset;
                }
            }

            return *this;
        }

        /** Addition operator.
         *
         * Incrementing beyond the end of the memory space will cause the
         * pointer to wrap around.
         * @param offset Element offset, may be negative
         * @return A copy of this iterator incremented by @a offset elements
         */
        iterator_generic operator+(difference_type offset) const
        {
            auto tmp = *this;
            return tmp += offset;
        }

        /** Subtraction assignment operator.
         *
         * Decrementing beyond the end of the memory space will cause the
         * pointer to wrap around.
         * @param offset Element offset, may be negative
         * @return A reference to this
         */
        iterator_generic& operator-=(difference_type offset)
        {
            return (*this) += -offset;
        }

        /** Subtraction operator.
         *
         * Decrementing beyond the end of the memory space will cause the
         * pointer to wrap around.
         * @param offset Element offset, may be negative
         * @return A copy of this iterator decremented by @a offset elements
         */
        iterator_generic operator-(difference_type offset) const
        {
            auto tmp = *this;
            return tmp -= offset;
        }

        /** Offset and dereference operator.
         *
         * In/decrementing beyond the end of the memory space will cause the
         * pointer to wrap around.
         * @param offset Element offset, may be negative
         * @return A reference to the element the iterator points to
         */
        reference operator[](difference_type offset) const
        {
            return *(*this + offset);
        }

        /** Difference in element count between two iterators.
         *
         * @param other Interator to count against
         * @return Difference count between this iterator and @a other
         */
        difference_type operator-(const iterator_generic& other) const
        {
            return current_ - other.current_;
        }

    private:
        friend class virtual_memory;

        using span = std::span<value_type,
                               std::tuple_size<virtual_memory::base::element_type>::value>;

        explicit iterator_generic(span data, bool is_end = false) :
            data_{data},
            current_{is_end ? data.end() : data.begin()}
        {}

        span data_;
        span::iterator current_;
    };

    using size_type              = base::element_type::size_type;           ///< Size type
    using difference_type        = base::element_type::difference_type;     ///< Pointer difference type
    using reference              = base::element_type::reference;           ///< Reference type
    using const_reference        = base::element_type::const_reference;     ///< Const reference type
    using pointer                = base::element_type::pointer;             ///< Pointer type
    using const_pointer          = base::element_type::const_pointer;       ///< Conts pointer type
    using iterator               = iterator_generic<false>;                 ///< Iterator type
    using const_iterator         = iterator_generic<true>;                  ///< Const iterator type
    using reverse_iterator       = std::reverse_iterator<iterator>;         ///< Reverse iterator type
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;   ///< Const reverse iterator type

    /** Constructor.
     *
     * Initialises the virtual memory by loading the program data and then
     * filling the remaining with the ternary operation on the two previous
     * elements.  Therefore it is an error if the program length is one ternary.
     *
     * @tparam InputIt Program data iterator type
     * @param first Iterator to first element in program data
     * @param last Iterator to one-past-the-end element in program data
     * @throw parse_exception Thrown if program length is less than 2 characters
     * @throw parse_exception Thrown if program length is greater than
     * math::ternary::max
     */
    template <typename InputIt>
    explicit virtual_memory(InputIt first, InputIt last) :
        mem_{std::make_unique<base::element_type>()}
    {
        const auto program_length = std::distance(first, last);
        if (program_length < 2) {
            throw parse_exception{"Program data must be at least 2 characters"};
        }

        if (static_cast<std::size_t>(program_length) > size()) {
            throw parse_exception{"Program data must be less than "
                                  "math::ternary::max"};
        }

        // Copy the program data in
        auto op_it = std::copy(first, last, mem_->begin());

        // Fill the remainder of the data space with the ternary op applied with
        // the previous two addresses
        for (; op_it != mem_->end(); ++op_it) {
            *op_it = (op_it-1)->op(*(op_it-2));
        }
    }

    /** Constructor.
     *
     * This equivalent to:
     * @code
     * virtual_memory(program_data.begin(), program_data.end())
     * @endcode
     * @tparam R Range type
     * @param program_data Program data
     * @throw parse_exception Thrown if program length is less than 2 characters
     * @throw parse_exception Thrown if program length is greater than
     * math::ternary::max
     */
    template <typename R>
    explicit virtual_memory(R&& program_data) :
        virtual_memory(program_data.begin(), program_data.end())
    {}

    /** Constructor.
     *
     * This equivalent to:
     * @code
     * virtual_memory(program_data.begin(), program_data.end())
     * @endcode
     * @tparam T List value type
     * @param program_data Program data
     * @throw parse_exception Thrown if program length is less than 2 characters
     * @throw parse_exception Thrown if program length is greater than
     * math::ternary::max
     */
    template <typename T>
    explicit virtual_memory(std::initializer_list<T> program_data) :
        virtual_memory(program_data.begin(), program_data.end())
    {}

    /** Move constructor.
     *
     * @param other Instance to move from
     */
    virtual_memory(virtual_memory&& other)  = default;

    /** Move assignment operator.
     *
     * @param other Instance to move from
     * @return A reference to this
     */
    virtual_memory& operator=(virtual_memory&& other) = default;

    virtual_memory(const virtual_memory& other) = delete;
    virtual_memory& operator=(const virtual_memory& other) = delete;

    /** Comparison operator.
     *
     * @param other Instance to compare against
     * @return Ordering
     */
    auto operator<=>(const virtual_memory& other) const = default;

    /** Returns a reference to the element at @a pos.
     *
     * If @a pos will exceeds the memory space, it will wrap around.
     * @param pos Offset from start of memory space
     * @return Reference to the element at @a pos (or equivalent wrapped)
     */
    reference operator[](size_type pos)
    {
        return iterator{*mem_}[pos];
    }

    /** Const-overload.
     *
     * If @a pos will exceeds the memory space, it will wrap around.
     * @param pos Offset from start of memory space
     * @return Reference to the element at @a pos (or equivalent wrapped)
     */
    const_reference operator[](size_type pos) const
    {
        return const_iterator{*mem_}[pos];
    }

    /** Returns the element at @a pos.
     *
     * Because this type's iterators wrap, this is the same as
     * operator[](size_type pos), and will never throw.
     * @param pos Offset from start of memory space
     * @return Reference to the element at @a pos (or equivalent wrapped)
     */
    reference at(size_type pos)
    {
        return (*this)[pos];
    }

    /** Const-overload.
     *
     * Because this type's iterators wrap, this is the same as
     * operator[](size_type pos) const, and will never throw.
     * @param pos Offset from start of memory space
     * @return Reference to the element at @a pos (or equivalent wrapped)
     */
    const_reference at(size_type pos) const
    {
        return (*this)[pos];
    }

    /** A iterator to the beginning of the memory space.
     *
     * @return Beginning iterator
     */
    iterator begin() noexcept
    {
        return iterator{*mem_};
    }

    /** Const-overload.
     *
     * @return Beginning iterator
     */
    const_iterator begin() const noexcept
    {
        return const_iterator{*mem_};
    }

    /** A const iterator to the beginning of the memory space.
     *
     * @return Beginning iterator
     */
    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    /** A iterator to one-past-the-end of the memory space.
     *
     * @note This iterator is @b never reached
     * @return One-past-the-end iterator
     */
    iterator end() noexcept
    {
        return iterator{*mem_, true};
    }

    /** Const-overload.
     *
     * @note This iterator is @b never reached
     * @return One-past-the-end iterator
     */
    const_iterator end() const noexcept
    {
        return const_iterator{*mem_, true};
    }

    /** A const iterator to one-past-the-end of the memory space.
     *
     * @note This iterator is @b never reached
     * @return One-past-the-end iterator
     */
    const_iterator cend() const noexcept
    {
        return end();
    }

    /** A reverse iterator to the end of the memory space.
     *
     * @return Reverse iterator
     */
    reverse_iterator rbegin() noexcept
    {
        return std::reverse_iterator{end()};
    }

    /** Const-overload.
     *
     * @return Reverse iterator
     */
    const_reverse_iterator rbegin() const noexcept
    {
        return std::reverse_iterator{end()};
    }

    /** A const reverse iterator to the end of the memory space.
     *
     * @return Reverse iterator
     */
    const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    /** A reverse iterator to one-before-the-beginning of the memory space.
     *
     * @note This iterator is @b never reached
     * @return Reverse iterator
     */
    reverse_iterator rend() noexcept
    {
        return std::reverse_iterator{begin()};
    }

    /** Const-overload.
     *
     * @note This iterator is @b never reached
     * @return Reverse iterator
     */
    const_reverse_iterator rend() const noexcept
    {
        return std::reverse_iterator{begin()};
    }

    /** A const reverse iterator to one-before-the-beginning of the memory
     * space.
     *
     * @note This iterator is @b never reached
     * @return Reverse iterator
     */
    const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    /** Returns true if empty.
     *
     * The memory space is a fixed size, so this @b always returns false.
     * @return False
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return false;
    }

    /** Returns the memory space size.
     *
     * The memory space is a fixed size, so this @b always returns
     * math::ternary::max.
     * @return Size
     */
    constexpr size_type size() const noexcept
    {
        return std::tuple_size<base::element_type>::value;
    }

    /** Returns the maximum memory space size.
     *
     * The memory space is a fixed size, so this @b always returns
     * math::ternary::max.
     * @return Size
     */
    constexpr size_type max_size() const noexcept
    {
        return size();
    }

private:
    base mem_;
};
}
