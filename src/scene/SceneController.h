// SceneController.h
//
//SceneController is the "document/model" that SceneView draws:
//   - it owns the GrammarEngine (all the building trees and world terminals),
//   - it owns the ground grid definition and the placement/snapping rules,
//   - it tracks which building is selected.

#ifndef SCENE_SCENECONTROLLER_H
#define SCENE_SCENECONTROLLER_H

#include <cstddef>
#include <functional>
#include <vector>

#include "scene/Primitives.h"          // GroundGrid
#include "grammar/GrammarEngine.h"     // engine + ShapeNode
#include "grammar/ShapeNode.h"
#include "grammar/BuildingRules.h"     // Parameters

namespace sg {

/**
 * The editable scene: a set of buildings on a ground grid, with one optionally
 * selected. Rendering and input live in SceneView; this class holds the state
 * and enforces the placement rules (grid confinement, non-overlap, snapping).
 */
class SceneController {
public:
    SceneController() = default;

    //building management (called by the UI sidebar):

    // Presets append a root; parameter edits replace only the selected root.
    // Invalid input leaves the scene unchanged. Returns this building's leaf count.
    std::size_t addBuilding(bool apartment, const building_rules::Parameters& parameters);
    std::size_t setBuilding(bool apartment, const building_rules::Parameters& parameters);

    // Read the selected building's family + parameters back out (for the sidebar
    // fields). Returns false when nothing is selected.
    bool getSelectedBuilding(bool& apartment, building_rules::Parameters& parameters) const;

    // Remove the selected building (restoring any walls its neighbours had
    // hidden). Returns true if a building was actually removed.
    bool deleteSelectedBuilding();

    //selection:

    int selectedBuildingId() const { return selectedRootId; }
    void selectBuilding(int buildingId);

    // Fires whenever the selection changes (so the sidebar can refresh its
    // fields). Separate from onChanged, which is about repainting the view.
    void onSelectionChanged(std::function<void()> callback);

    // Fires whenever the scene's geometry/selection changes and the view should
    // repaint. SceneView wires this to its own redraw().
    void onChanged(std::function<void()> callback);

    //dragging (placement policy used by SceneView while moving):

    // Snap a proposed ground point to the grid, keep the selected building
    // inside it, and resolve structural volume overlaps to surface contact.
    // Returns the point unchanged when nothing is selected.
    V3 snapToGrid(const V3& point) const;

    // Move the selected building to a ground placement (already snapped), then
    // refresh selection-dependent UI because joining may change its tree.
    void moveSelectedTo(const V3& position);

    //read access for rendering + picking (used by SceneView:

    const GrammarEngine& engineRef() const { return engine; }
    const primitives::GroundGrid& gridRef() const { return grid; }

    // World position of the selected root (for computing the drag grab offset).
    // Returns false when nothing is selected.
    bool selectedRootPosition(V3& outPosition) const;

    // World-space bounding box of the selected building's full geometry (used by
    // the view to draw the selection outline). Returns false when nothing is
    // selected. "Full" geometry means hidden/joined faces are still included, so
    // the outline stays stable even when a shared wall is removed.
    bool selectedBuildingBounds(AABB& outBounds) const;

    //derivation tree (Checkpoint 3):
    //
    // A node inside a building's derivation tree is addressed by its "path": the
    // sequence of child indices from the selected building's root down to it. An empty path = the root itself.

    // The selected building's root node, so the tree view can walk its children
    // to build the list. Returns nullptr when nothing is selected.
    const ShapeNode* derivationRoot() const;

    // Ask the view to outline the node at `path` within the selected building.
    // An out-of-range path clears the highlight. Fires onChanged so it repaints.
    void highlightNode(const std::vector<int>& path);

    // Stop highlighting any node (e.g. when the tree selection is cleared).
    void clearNodeHighlight();

    // World-space bounding box of the currently highlighted node's subtree, for
    // the view's node outline. Returns false when no node is highlighted (or the
    // path no longer resolves, e.g. after the building was regenerated).
    bool highlightedNodeBounds(AABB& outBounds) const;

private:
    GrammarEngine engine;              // all building trees + world terminals
    primitives::GroundGrid grid;       // ground reference grid + snap unit

    // Stable owner ID rather than a pointer into the roots vector.
    int selectedRootId = -1;

    // Derivation-tree node to outline, as a child-index path from the selected
    // building's root. Empty + highlightActive=false means "no node highlighted".
    std::vector<int> highlightPath;
    bool highlightActive = false;

    std::function<void()> selectionChanged; // -> refresh sidebar fields
    std::function<void()> changed;          // -> repaint the view

    // Fire the "scene changed, please repaint" callback if one is registered.
    void notifyChanged();
};

} // namespace sg

#endif // SCENE_SCENECONTROLLER_H
