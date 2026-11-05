// SceneController.cpp: building management, placement, and selection.
// Code moved here from SceneView so that the view stays focused on rendering + input

#include "scene/SceneController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sg {

namespace {
// World-space bounding box of some terminals. With a buildingId >= 0, only the terminals belonging to that building are measured- otherwise all are.
// Terminals can be rotated, so each box corner is rotated into world space before it grows the bounds.
AABB terminalBounds(const std::vector<ShapeNode>& terminals, int buildingId = -1) {
    AABB bounds;
    for (const ShapeNode& terminal : terminals) {
        if (buildingId >= 0 && terminal.buildingId != buildingId) continue;
        V3 center = terminal.position, half = terminal.size;
        half = half * 0.5f;
        M33 rotation = terminal.rotation;
        for (int x : {-1, 1}) for (int y : {-1, 1}) for (int z : {-1, 1})
            bounds.AddPoint(center + rotation * V3(x*half[0], y*half[1], z*half[2]));
    }
    return bounds;
}

// Reject a building whose footprint reaches past the grid, with a small epsilon
// so a building sitting exactly on the edge is still allowed.
void requireInsideGrid(AABB bounds, float limit) {
    if (bounds.minC[0] < -limit-1e-4f || bounds.maxC[0] > limit+1e-4f ||
        bounds.minC[2] < -limit-1e-4f || bounds.maxC[2] > limit+1e-4f)
        throw std::invalid_argument("This size extends beyond the grid. Move the building inward or reduce its width/depth.");
}
} // namespace

void SceneController::notifyChanged() {
    if (changed) changed();
}

std::size_t SceneController::addBuilding(bool apartment, const building_rules::Parameters& parameters) {
    ShapeNode root = apartment ? building_rules::makeApartment(parameters)
                               : building_rules::makeOffice(parameters);
    engine.GenerateTree(&root);
    const auto pieces = engine.TerminalsForRoot(root);
    AABB bounds = terminalBounds(pieces);
    requireInsideGrid(bounds, grid.halfExtent);

    // Find a free grid location, trying the origin and then nearby X positions
    // before starting another row. Existing buildings keep their placement.
    std::vector<AABB> occupied;
    for (const ShapeNode& existing : engine.roots)
        occupied.push_back(terminalBounds(engine.TerminalsForRoot(existing)));
    std::vector<float> coordinates{0};
    const int cells = static_cast<int>(std::floor(grid.halfExtent / grid.spacing));
    for (int i = 1; i <= cells; ++i) {
        coordinates.push_back(i * grid.spacing);
        coordinates.push_back(-i * grid.spacing);
    }
    bool found = false;
    V3 placement(0, 0, 0);
    for (float z : coordinates) {
        for (float x : coordinates) {
            if (bounds.minC[0]+x < -grid.halfExtent || bounds.maxC[0]+x > grid.halfExtent ||
                bounds.minC[2]+z < -grid.halfExtent || bounds.maxC[2]+z > grid.halfExtent) continue;
            bool overlaps = false;
            for (AABB other : occupied) {
                if (bounds.maxC[0]+x > other.minC[0]-grid.spacing &&
                    bounds.minC[0]+x < other.maxC[0]+grid.spacing &&
                    bounds.maxC[2]+z > other.minC[2]-grid.spacing &&
                    bounds.minC[2]+z < other.maxC[2]+grid.spacing) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) continue;
            placement = V3(x, 0, z);
            found = true;
            break;
        }
        if (found) break;
    }
    if (!found) throw std::invalid_argument("The grid has no free space for another building.");
    root.position = root.position + placement;
    const int buildingId = engine.AddRoot(std::move(root));
    selectBuilding(buildingId);
    notifyChanged();
    return pieces.size();
}

std::size_t SceneController::setBuilding(bool apartment, const building_rules::Parameters& parameters) {
    const ShapeNode* current = engine.FindRoot(selectedRootId);
    if (!current) throw std::invalid_argument("Select a building before applying changes.");
    // Build a candidate with the same placement/orientation. Commit only after
    // validation, preserving every other root and its generated geometry.
    ShapeNode root = apartment ? building_rules::makeApartment(parameters)
                               : building_rules::makeOffice(parameters);
    V3 previousPosition = current->position, previousSize = current->size;
    root.position = V3(previousPosition[0], previousPosition[1]-previousSize[1]/2+root.size[1]/2,
                       previousPosition[2]);
    root.rotation = current->rotation;
    engine.GenerateTree(&root);
    const auto pieces = engine.TerminalsForRoot(root);
    requireInsideGrid(terminalBounds(pieces), grid.halfExtent);
    engine.ReplaceRoot(selectedRootId, std::move(root));
    if (selectionChanged) selectionChanged();
    notifyChanged();
    return pieces.size();
}

bool SceneController::getSelectedBuilding(bool& apartment, building_rules::Parameters& parameters) const {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return false;
    V3 size = root->size;
    apartment = root->symbol == Symbol::Apartment;
    parameters.floors = root->building.floors;
    parameters.bays = root->building.bays;
    parameters.width = size[0];
    parameters.depth = size[2];
    parameters.floorHeight = (size[1]-building_rules::roofHeight) / parameters.floors;
    parameters.balconies = root->building.balconies;
    return true;
}

void SceneController::selectBuilding(int buildingId) {
    if (buildingId != -1 && !engine.FindRoot(buildingId))
        throw std::invalid_argument("Building no longer exists.");
    if (selectedRootId == buildingId) return;
    selectedRootId = buildingId;
    if (selectionChanged) selectionChanged();
    notifyChanged();
}

bool SceneController::deleteSelectedBuilding() {
    if (selectedRootId < 0) return false;
    const bool removed = engine.RemoveRoot(selectedRootId);
    selectBuilding(-1); // Clear highlighting/drag state and refresh the sidebar.
    return removed;
}

void SceneController::onSelectionChanged(std::function<void()> callback) {
    selectionChanged = std::move(callback);
}

void SceneController::onChanged(std::function<void()> callback) {
    changed = std::move(callback);
}

bool SceneController::selectedRootPosition(V3& outPosition) const {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return false;
    outPosition = root->position;
    return true;
}

bool SceneController::selectedBuildingBounds(AABB& outBounds) const {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return false;
    // Full geometry keeps the selected outline stable even when a join hides
    // a whole side (or another building completely contains this root).
    outBounds = terminalBounds(engine.TerminalsForRoot(*root));
    return true;
}

// Snap a ground point to the nearest grid intersection, then confine it so the WHOLE selected building stays inside the grid.
V3 SceneController::snapToGrid(const V3& point) const {
    V3 p = point; // local mutable copy
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return p;
    const float step = grid.spacing;            // one grid cell, the snap unit
    float sx = std::round(p[0] / step) * step;  // round to nearest grid step
    float sz = std::round(p[2] / step) * step;

    // keep the placement inside the grid's extent so you can't drag off the edge
    AABB bounds = terminalBounds(engine.TerminalsForRoot(*root));
    V3 rootPosition = root->position;
    // Use asymmetric bounds (balconies/canopies project forward) and round the
    // allowed interval inward so clamping also preserves grid alignment.
    const float minX = std::ceil((-grid.halfExtent-bounds.minC[0]+rootPosition[0])/step)*step;
    const float maxX = std::floor((grid.halfExtent-bounds.maxC[0]+rootPosition[0])/step)*step;
    const float minZ = std::ceil((-grid.halfExtent-bounds.minC[2]+rootPosition[2])/step)*step;
    const float maxZ = std::floor((grid.halfExtent-bounds.maxC[2]+rootPosition[2])/step)*step;
    sx = std::max(minX, std::min(maxX, sx));
    sz = std::max(minZ, std::min(maxZ, sz));

    return V3(sx, p[1], sz);
}

void SceneController::moveSelectedTo(const V3& position) {
    if (selectedRootId < 0) return;
    engine.MoveRoot(selectedRootId, position);
    notifyChanged();
}

} // namespace sg
