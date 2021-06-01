/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#pragma once

#include "malbolge/ui/terminal/panes/functional_pane.hpp"

namespace malbolge
{
namespace ui
{
namespace terminal
{
/** The output pane.
 *
 * This provides the user:
 * - An area to view the program's output
 * - An option to save the output to disk
 * - An option to clear the output
 */
class output_pane : public functional_pane
{
public:
    /** Default constructor.
     */
    explicit output_pane();

    /** Destructor.
     */
    virtual ~output_pane() override = default;

    [[nodiscard]]
    virtual std::string_view name() const noexcept override;

    /** Appends a new character from the program to the output data.
     *
     * @param c New program output character
     */
    void append(char c) noexcept;

protected:
     virtual void draw() override;

private:
    std::string output_;
};
}
}
}
