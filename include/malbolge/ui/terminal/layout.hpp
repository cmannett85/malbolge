/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/ui/terminal/pane.hpp"

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Manages the layout of panes inside the terminal.
 *
 * This class can be moved but not copied.
 */
class layout
{
public:
    using line_type = pane::coordinate::line_type;
    using col_type = pane::coordinate::col_type;

    /** Alias for an owning pointer.
     */
    using pane_ptr = std::shared_ptr<pane>;

    /** The direction to split a pane into two.
     */
    enum class split_direction {
        HORIZONTAL,     ///< Horizontal
        VERTICAL,       ///< Vertical
        NUM_DIRECTIONS  ///< Number of directions
    };

    /** Constructor.
     *
     *  @param lines Number of lines in the whole terminal
     *  @param cols Number of columns in the whole terminal
     */
    explicit layout(line_type lines, col_type cols);

    /** Replace the existing pane (or empty root) with @a new_pane.
     *
     * @note A refresh is not performed automatically
     * @param new_pane New pane
     * @param existing_pane An existing pane to replace, or null to replace the
     * root
     * @exception basic_exception Thrown if @a new_pane is null, or is
     * @a existing_pane is non-null but cannot be found in the layout
     */
    void replace(pane_ptr new_pane, pane_ptr existing_pane = {});

    /** Split @a parent and add @a new_pane to the opened slot.
     *
     * The parent of a split is either the top or left pane depending on @a dir.
     * @note A refresh is not performed automatically
     * @param new_pane New pane
     * @param dir Direction to split @a parent
     * @param parent Pane to split
     * @exception basic_exception Thrown if @a new_pane or @a parent is null, or
     * if @a parent cannot be found in the layout
     * @exception basic_exception Thrown if @a parent cannot be split anymore
     */
    void split(pane_ptr new_pane, split_direction dir, pane_ptr parent);

    /** Inform the layout engine that the terminal has been resized.
     *
     * This will trigger a redraw of all child panes.
     * @param lines New line count
     * @param cols New column count
     */
    void terminal_resize(line_type lines, col_type cols);

    /** Trigger a redraw of all the panes in the layout.
     *
     * This should be called after the layout has been modified.
     */
    void refresh() noexcept
    {
        refresh(root_);
    }

    layout(layout&&) = default;
    layout& operator=(layout&&) = default;
    layout(const layout&) = delete;
    layout& operator=(const layout&) = delete;

private:
    struct node;
    using node_ptr = std::shared_ptr<node>;

    void recalculate_dimensions(node_ptr start_node);
    void refresh(node_ptr start_node) noexcept;

    [[nodiscard]]
    node_ptr find(std::shared_ptr<pane> target, node_ptr start_node) noexcept;

    [[nodiscard]]
    node_ptr find(std::shared_ptr<pane> target) noexcept
    {
        return find(std::move(target), root_);
    }

    line_type lines_;
    col_type cols_;
    node_ptr root_;
};

/** Returns a textual equivalent to a layout::split_direction.
 *
 * @param dir Split direction
 * @return String
 */
std::string_view to_string(layout::split_direction dir) noexcept;
}
}
}
