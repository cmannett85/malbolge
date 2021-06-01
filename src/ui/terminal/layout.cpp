/* Cam Mannett 2021
 *
 * See LICENSE file
 */

#include "malbolge/exception.hpp"
#include "malbolge/ui/terminal/layout.hpp"
#include "malbolge/ui/terminal/pane.hpp"
#include "malbolge/utility/string_view_ops.hpp"

#include <array>
#include <algorithm>

using namespace malbolge;
using namespace malbolge::ui;
using namespace malbolge::utility::string_view_ops;
using namespace std::string_literals;

struct terminal::layout::node
{
    struct split_info
    {
        node_ptr node;
        split_direction dir;
    };

    explicit node(pane_ptr p = {}, node_ptr par = {}) :
        pane{std::move(p)},
        parent{std::move(par)}
    {}

    pane_ptr pane;
    node_ptr parent;
    std::array<split_info, traits::to_underlying_type(split_direction::NUM_DIRECTIONS)> children;
};

terminal::layout::layout(line_type lines, col_type cols) :
    lines_{lines},
    cols_{cols},
    root_{std::make_shared<node>()}
{}

void terminal::layout::replace(pane_ptr new_pane, pane_ptr existing_pane)
{
    if (!new_pane) {
        throw basic_exception{"New pane is null"};
    }

    // Find the pane to replace
    auto target_node = root_;
    if (existing_pane) {
        target_node = find(existing_pane);
        if (!target_node) {
            throw basic_exception{"Cannot find pane \""s + existing_pane->name() +
                                  "\" in layout"};
        }
    }

    target_node->pane = std::move(new_pane);
    recalculate_dimensions(std::move(target_node));
}

void terminal::layout::split(pane_ptr new_pane,
                             split_direction dir,
                             pane_ptr parent)
{
    if (!new_pane || !parent) {
        throw basic_exception{"New pane and parent must be non-null"};
    }

    // Find the parent to split
    auto target_node = find(parent);
    if (!target_node) {
        throw basic_exception{"Cannot find pane \""s + parent->name() +
                              "\" in layout"};
    }

    auto empty_child = std::find_if(target_node->children.begin(),
                                    target_node->children.end(),
                                    [](auto& c) { return !c.node; });
    if (empty_child == target_node->children.end()) {
        throw basic_exception{"Pane cannot be split any further"};
    }
    if (std::ranges::any_of(target_node->children, [&](auto&& c) { return c.node && c.dir == dir; })) {
        throw basic_exception{"Pane already split in "s + to_string(dir) + "direction"};
    }

    empty_child->node = std::make_shared<node>(std::move(new_pane),
                                               target_node);
    empty_child->dir = dir;

    recalculate_dimensions(std::move(target_node));
}

void terminal::layout::terminal_resize(line_type lines, col_type cols)
{
    lines_ = lines;
    cols_ = cols;
    recalculate_dimensions(root_);
    refresh();
}

void terminal::layout::recalculate_dimensions(node_ptr start_node)
{
    if (!start_node->pane) {
        throw basic_exception{"No pane in node for dimension recalculation"};
    }

    if (start_node == root_) {
        root_->pane->set_dimensions({lines_, cols_}, {});
    }

    for (auto& si : start_node->children) {
        if (si.node) {
            auto old_size = start_node->pane->size();
            auto old_pos = start_node->pane->position();
            auto new_size = old_size;
            auto new_pos = old_pos;

            if (si.dir == split_direction::HORIZONTAL) {
                new_size.lines = std::min<line_type>(new_size.lines / 2u,
                                                     si.node->pane->maximum_size().lines);
                old_size.lines -= new_size.lines;
                new_pos.lines += old_size.lines;
            } else {
                new_size.cols = std::min<col_type>(new_size.cols / 2u,
                                                   si.node->pane->maximum_size().cols);
                old_size.cols -= new_size.cols;
                new_pos.cols += old_size.cols;
            }
            // Set new parent size
            start_node->pane->set_dimensions(old_size, old_pos);

            // Set new child size
            si.node->pane->set_dimensions(new_size, new_pos);

            // Update grandchildren
            recalculate_dimensions(si.node);
        }
    }
}

void terminal::layout::refresh(node_ptr start_node) noexcept
{
    // Iterate through the children depth-first and trigger a refresh in each
    // of their panes
    start_node->pane->refresh();
    for (auto& si : start_node->children) {
        if (si.node) {
            refresh(si.node);
        }
    }
}

terminal::layout::node_ptr
terminal::layout::find(std::shared_ptr<pane> target,
                       node_ptr start_node) noexcept
{
    if (!start_node || start_node->pane == target) {
        return start_node;
    }

    // Recursively depth-first search through the tree.  As layout changes are
    // infrequent, performance isn't a priority
    for (auto& c : start_node->children) {
        auto result = find(target, c.node);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

std::string_view terminal::to_string(layout::split_direction dir) noexcept
{
    switch (dir) {
    case layout::split_direction::HORIZONTAL:
        return "Horizontal";
    case layout::split_direction::VERTICAL:
        return "Vertical";
    default:
        return "Unknown";
    }
}
