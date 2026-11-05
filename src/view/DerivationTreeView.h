// DerivationTreeView.h: the Checkpoint 3 derivation-tree panel.
//
// A read-only tree that mirrors the SELECTED building's derivation tree
// (the ShapeNode hierarchy the grammar produced). Picking a node in the tree
// tells the SceneController to highlight that node's component in the 3D view.

#ifndef VIEW_DERIVATIONTREEVIEW_H
#define VIEW_DERIVATIONTREEVIEW_H

#include <FL/Fl_Tree.H>

#include <vector>

#include "scene/SceneController.h"
#include "grammar/ShapeNode.h"

namespace sg {

/**
 * FLTK tree panel showing the selected building's derivation tree.
 *
 *  - refresh() rebuilds the list from the controller's current selection.
 *  - clicking a node calls controller.highlightNode(path), where `path` is the
 *    child-index route from the building root to that node (ShapeNode has no
 *    usable id under the current rules, so a path is how we name a node).
 *
 * Wire this up in main(): construct it in the sidebar, then make the scene's
 * onSelectionChanged() (and onChanged, after edits) call refresh() so the tree
 * follows whatever building is selected.
 */
class DerivationTreeView : public Fl_Tree {
public:
    DerivationTreeView(SceneController& controller, int x, int y, int w, int h,
                       const char* label = nullptr);

    // Rebuild the tree from the currently selected building (empty if none).
    void refresh();

private:
    SceneController& controller;

    // Each tree item owns a heap-allocated path (vector<int>) as its user_data,
    // so a click can tell the controller exactly which node to highlight. We
    // keep the vectors alive here and free them when the tree is rebuilt.
    std::vector<std::vector<int>*> itemPaths;

    // Recursively add `node` (reached by `path`) and its children under `parent`.
    void addNode(const ShapeNode& node, const std::vector<int>& path,
                 const std::string& parentPath);

    // Free every path we allocated for the previous tree contents.
    void clearItemPaths();

    // FLTK callback: routes a node click to highlightSelectedItem().
    static void onItemClicked(Fl_Widget* widget, void* data);
    void highlightSelectedItem();
};

} // namespace sg

#endif // VIEW_DERIVATIONTREEVIEW_H
