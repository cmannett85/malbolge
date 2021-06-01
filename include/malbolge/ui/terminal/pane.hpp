/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <functional>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Base class for all window panes.
 *
 * This class does almost nothing, it is a really just an interface for the
 * layout engine - see functional_pane for something more easily derivable.
 *
 * This class can be moved, but not copied.
 */
class pane
{
public:
    /** Represents a coordinate or size.
     */
    struct coordinate
    {
        /** Line unit type.
         */
        using line_type = std::uint16_t;

        /** Column unit type.
         */
        using col_type = std::uint16_t;

        /** Constructor
         *
         * @param l Number of lines
         * @param c Number of columns
         */
        coordinate(line_type l = 0, col_type c = 0) noexcept :
            lines{l},
            cols{c}
        {}

        line_type lines;  ///< Y-coordinate in lines
        col_type cols;   ///< X-coordinate in columns
    };

    /** Default constructor.
     */
    explicit pane() = default;

    pane(pane&& other) noexcept = default;
    pane& operator=(pane&& other) noexcept = default;
    pane(const pane&) noexcept = delete;
    pane& operator=(const pane&) noexcept = delete;

    /** Destructor.
     */
    virtual ~pane() = default;

    /** Returns the current size.
     *
     * @return Size
     */
    [[nodiscard]]
    coordinate size() const noexcept
    {
        return size_;
    }

    /** Returns the largest size the pane is allowed to be.
     *
     * The default implementation returns the maximum values possible.
     * @return Largest allowable size
     */
    [[nodiscard]]
    virtual coordinate maximum_size() const noexcept;

    /** Returns the current position.
     *
     * @return Position
     */
    [[nodiscard]]
    coordinate position() const noexcept
    {
        return pos_;
    }

    /** Returns the name of the pane.
     *
     * This is used for any UI elements that need it, e.g. pane tab labels.
     * @return Pane name
     */
    [[nodiscard]]
    virtual std::string_view name() const noexcept = 0;

    /** Updates the size and position of the pane.
     *
     * This sets the size() and position() members, before calling
     * dimensions_updated().  This will @em not call refresh() after the resize.
     * @param size Size of the pane, zero in any axis fills the terminal in that
     * axis
     * @param pos Absolute position of pane origin
     * @exception basic_exception Thrown if the pane could not be resized
     */
    void set_dimensions(coordinate size, coordinate pos);

    /** Trigger a redraw of the pane.
     *
     *  @exception basic_exception Thrown if the pane could not render
     */
    virtual void refresh() = 0;

protected:
    /** Derived types must override this to be notified when the pane dimensions
     * change.
     *
     * @exception basic_exception Thrown if the pane could not be resized
     */
    virtual void dimensions_updated() = 0;

private:
    coordinate size_;
    coordinate pos_;
};
}
}
}
