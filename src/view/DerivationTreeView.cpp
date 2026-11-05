// DerivationTreeView.cpp
// Checkpoint 3: mirrors the selected building's tree and highlights a node's component in the 3D view when it is clicked.

#include "view/DerivationTreeView.h"

#include <FL/Fl_Tree_Item.H>

#include <string>

namespace sg {

namespace {
// Human-readable name for a grammar Symbol, used as the tree item's label.
const char* symbolName(Symbol symbol) {
    switch (symbol) {
    case Symbol::Office:     return "Office";
    case Symbol::Apartment:  return "Apartment";
    case Symbol::Floor:      return "Floor";
    case Symbol::WallRegion: return "WallRegion";
    case Symbol::BayGroup:   return "BayGroup";
    case Symbol::Bay:        return "Bay";
    case Symbol::Entrance:   return "Entrance";
    case Symbol::Balcony:    return "Balcony";
    case Symbol::Roof:       return "Roof";
    case Symbol::Facade:     return "Facade";
    case Symbol::Window:     return "Window";
    case Symbol::Slab:       return "Slab";
    }
    return "Node";
}
} // namespace

DerivationTreeView::DerivationTreeView(SceneController& controller, int x, int y, int w, int h,
                                       const char* label)
    : Fl_Tree(x, y, w, h, label), controller(controller) {
    // Show the tree bare: no synthetic "ROOT" line, just the building's nodes.
    showroot(0);
    // A single click on an item fires our callback (not just double-click).
    callback(onItemClicked, this);
    when(FL_WHEN_CHANGED);
}

void DerivationTreeView::clearItemPaths() {
    for (std::vector<int>* path : itemPaths) delete path;
    itemPaths.clear();
}

// Fl_Tree addresses items by a '/'-separated string path that must be UNIQUE
// among siblings. Node symbols repeat (many "Floor"s, "Bay"s, ...), so we append
// the child index (e.g. "Floor #2") to keep each item path distinct. The real
// node identity we care about -- the child-index route -- is stored separately
// as the item's user_data for the click handler.
void DerivationTreeView::addNode(const ShapeNode& node, const std::vector<int>& path,
                                 const std::string& parentPath) {
    // Build this item's display/addressing segment.
    std::string segment = symbolName(node.symbol);
    if (!path.empty()) segment += " #" + std::to_string(path.back() + 1);
    const std::string itemPath = parentPath.empty() ? segment : parentPath + "/" + segment;

    Fl_Tree_Item* item = add(itemPath.c_str());
    if (item) {
        // Remember the child-index route to this node so a click can highlight it.
        auto* stored = new std::vector<int>(path);
        itemPaths.push_back(stored);
        item->user_data(stored);
    }

    // Recurse into children, extending the path with each child's index.
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        std::vector<int> childPath = path;
        childPath.push_back(static_cast<int>(i));
        addNode(node.children[i], childPath, itemPath);
    }
}

void DerivationTreeView::refresh() {
    clear();           // drop all FLTK items
    clearItemPaths();  // free the paths that belonged to them

    const ShapeNode* root = controller.derivationRoot();
    if (!root) {
        // Nothing selected: leave the tree empty (the panel label explains it).
        redraw();
        return;
    }

    // The empty path addresses the root itself; children extend it by index.
    addNode(*root, std::vector<int>{}, std::string());
    redraw();
}

void DerivationTreeView::onItemClicked(Fl_Widget* widget, void* data) {
    static_cast<DerivationTreeView*>(data)->highlightSelectedItem();
}

void DerivationTreeView::highlightSelectedItem() {
    // Only react to the item that actually changed, and only when it became
    // selected (Fl_Tree also reports deselection/open/close through the callback).
    Fl_Tree_Item* item = callback_item();
    if (!item || callback_reason() != FL_TREE_REASON_SELECTED) return;

    const auto* path = static_cast<const std::vector<int>*>(item->user_data());
    if (path) controller.highlightNode(*path);
}

} // namespace sg
