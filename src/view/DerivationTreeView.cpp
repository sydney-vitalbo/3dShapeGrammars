// DerivationTreeView.cpp
// Checkpoint 3: mirrors the selected building's tree and highlights a node's component in the 3D view when it is clicked.

#include "view/DerivationTreeView.h"

#include <FL/Fl_Tree_Item.H>

#include <string>
#include <map>

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
    case Symbol::JoinedUnit: return "Joined unit";
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

// Add directly to parent items so identical sibling labels stay distinct.
// Child-index paths in user_data identify nodes for highlighting/detachment.
void DerivationTreeView::addNode(const ShapeNode& node, const std::vector<int>& path,
                                 Fl_Tree_Item* parent, int typeIndex) {
    std::string label=symbolName(node.symbol);
    if(!path.empty()) label += " " + std::to_string(typeIndex);
    Fl_Tree_Item* item = parent ? add(parent,label.c_str()) : add(label.c_str());
    if (!item) return;
    if (item) {
        // Remember the child-index route to this node so a click can highlight it.
        auto* stored = new std::vector<int>(path);
        itemPaths.push_back(stored);
        item->user_data(stored);
    }

    // Display numbering starts at 1 separately for each sibling symbol.
    // Actual child-index paths remain unchanged for highlighting/detachment.
    std::map<Symbol,int> counts;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        std::vector<int> childPath = path;
        childPath.push_back(static_cast<int>(i));
        addNode(node.children[i], childPath, item, ++counts[node.children[i].symbol]);
    }
    close(item,0); // Start every branch collapsed, without selection callbacks.
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
    addNode(*root, std::vector<int>{}, nullptr);
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
