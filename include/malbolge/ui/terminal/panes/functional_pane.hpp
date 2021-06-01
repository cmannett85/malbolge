/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/ui/terminal/shortcut_key.hpp"
#include "malbolge/ui/terminal/pane.hpp"

#include <ncurses.h>

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** Base class for all 'functional' (i.e. not a command pane) panes.
 *
 * This class can be moved, but not copied.
 */
class functional_pane : public pane
{
public:
    /** Pair-like object containing a shortcut key and its description.
     *
     * This is used in the UI to define and describe the shortcuts to the user.
     */
    struct key_description {
        shortcut_key key;               ///< Shortcut key
        std::string_view description;   ///< Description
    };

    /** Key list type.
     */
    using key_list_type = std::vector<key_description>;

    /** Default constructor.
     *
     * Sets the pane inactive.
     */
    explicit functional_pane() :
        active_{false}
    {}

    /** Destructor.
     */
    virtual ~functional_pane() = default;

    virtual void refresh() override;

    /** Returns the key bindings for this pane.
     *
     * The default implementation returns an empty list.
     * @return Key binding list
     */
    [[nodiscard]]
    virtual const key_list_type& key_bindings() const noexcept;

    /** Handles a key press for this pane.
     *
     * The default implementation does nothing.
     * @param key Key press
     */
    virtual void key_press(int key);

    /** Returns the active state of the pane.
     *
     * @return True if active
     */
    [[nodiscard]]
    bool active() const noexcept
    {
        return active_;
    }

    /** Sets the pane active or inactive.
     *
     * An active pane will render differently and receive keyboard input.
     * @param active True to set active
     * @param refresh True to trigger a refresh after the change, a no-op if no
     * change has occurred
     */
    void set_active(bool active, bool refresh = false);

protected:
    virtual void dimensions_updated() override;

    /** Derived types should override this method to draw their content within
     * the pane.
     *
     * @note Do @b not call refresh() from this method as you will enter an
     * infinite loop!
     */
    virtual void draw() = 0;

    /** Returns the underlying window.
     *
     * @return NCurses window
     */
    [[nodiscard]]
    WINDOW* window() noexcept
    {
        return pane_win_.get();
    }

private:
    using window_ptr = std::unique_ptr<WINDOW, std::function<void (WINDOW*)>>;

    window_ptr win_;
    window_ptr title_win_;
    window_ptr pane_win_;

    bool active_;
};
}
}
}
