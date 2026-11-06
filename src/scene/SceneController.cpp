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

// Structural root bounds are the boxes used by GrammarEngine joining. Facade
// projections are deliberately excluded so they may extend over a neighbour
// without being treated as intersecting building volumes.
AABB rootBoundsAt(const ShapeNode& root, V3 position) {
    AABB bounds;
    V3 size = root.size;
    M33 rotation = root.rotation;
    for (int x : {-1, 1}) for (int y : {-1, 1}) for (int z : {-1, 1})
        bounds.AddPoint(position + rotation *
            V3(x*size[0]/2, y*size[1]/2, z*size[2]/2));
    return bounds;
}

bool volumeOverlap(AABB a, AABB b) {
    constexpr float epsilon = 1e-4f;
    for (int axis = 0; axis < 3; ++axis)
        if (std::min(a.maxC[axis], b.maxC[axis]) -
            std::max(a.minC[axis], b.minC[axis]) <= epsilon) return false;
    return true;
}

// Reject a building whose footprint reaches past the grid, with a small epsilon
// so a building sitting exactly on the edge is still allowed.
void requireInsideGrid(AABB bounds, float limit) {
    if (bounds.minC[0] < -limit-1e-4f || bounds.maxC[0] > limit+1e-4f ||
        bounds.minC[2] < -limit-1e-4f || bounds.maxC[2] > limit+1e-4f)
        throw std::invalid_argument("This size extends beyond the grid. Move the building inward or reduce its width/depth.");
}

// Follow a child-index path from `root` to the node it names, accumulating the
// world transform as we descend (positions compose through each parent's
// rotation, matching how the engine collects terminals). Returns the node, plus
// its world position/rotation via the out-params. nullptr if the path is invalid.
const ShapeNode* resolvePath(const ShapeNode& root, const std::vector<int>& path,
                             V3& outWorldPos, M33& outWorldRot) {
    const ShapeNode* node = &root;
    V3 worldPos = root.position;          // root position is already world space
    M33 worldRot = root.rotation;
    for (int index : path) {
        if (index < 0 || index >= static_cast<int>(node->children.size())) return nullptr;
        const ShapeNode& child = node->children[index];
        // childWorld = parentWorld + parentRot * childLocalPos; rotations compose.
        worldPos = worldPos + worldRot * child.position;
        worldRot = worldRot * child.rotation;
        node = &child;
    }
    outWorldPos = worldPos;
    outWorldRot = worldRot;
    return node;
}

// World-space AABB of a node's whole subtree, given the node's own world pose.
// Walks every descendant the same way the engine does, growing the box over the
// rotated corners of each piece (terminal or not, so an unexpanded/nonterminal
// scope still outlines sensibly).
void growSubtreeBounds(const ShapeNode& node, V3 worldPos, M33 worldRot, AABB& bounds) {
    V3 half = node.size; half = half * 0.5f;
    M33 rot = worldRot;
    for (int x : {-1, 1}) for (int y : {-1, 1}) for (int z : {-1, 1})
        bounds.AddPoint(worldPos + rot * V3(x*half[0], y*half[1], z*half[2]));
    for (const ShapeNode& child : node.children) {
        V3 childPos = worldPos + rot * child.position;
        M33 childRot = rot * child.rotation;
        growSubtreeBounds(child, childPos, childRot, bounds);
    }
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
    // The tree was regenerated, so tree panel reselects nothing after refresh()
    highlightActive = false;
    highlightPath.clear();
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
    // A node highlight belongs to the previously selected building's tree, so it
    // no longer applies once a different building (or nothing) is selected.
    highlightActive = false;
    highlightPath.clear();
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

const ShapeNode* SceneController::derivationRoot() const {
    return engine.FindRoot(selectedRootId);
}

void SceneController::highlightNode(const std::vector<int>& path) {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    // Validate the path against the current tree; an unresolved path clears the
    // highlight rather than leaving a stale box behind.
    V3 pos; M33 rot;
    if (root && resolvePath(*root, path, pos, rot) != nullptr) {
        highlightPath = path;
        highlightActive = true;
    } else {
        highlightPath.clear();
        highlightActive = false;
    }
    notifyChanged();
}

void SceneController::clearNodeHighlight() {
    if (!highlightActive && highlightPath.empty()) return;
    highlightPath.clear();
    highlightActive = false;
    notifyChanged();
}

bool SceneController::highlightedNodeBounds(AABB& outBounds) const {
    if (!highlightActive) return false;
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return false;
    V3 worldPos; M33 worldRot;
    const ShapeNode* node = resolvePath(*root, highlightPath, worldPos, worldRot);
    if (!node) return false;
    AABB bounds;
    growSubtreeBounds(*node, worldPos, worldRot, bounds);
    outBounds = bounds;
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

    // A snapped request may land inside another structural root. Push it to the
    // nearest horizontal face, preferring the side from which it approached.
    // Repeat because resolving one obstruction can encounter another building.
    const float requestedX = sx, requestedZ = sz;
    AABB oldBounds = rootBoundsAt(*root, rootPosition);
    const int maxCorrections = std::max(1, static_cast<int>(engine.roots.size()) * 4);
    for (int pass = 0; pass < maxCorrections; ++pass) {
        bool corrected = false;
        for (const ShapeNode& other : engine.roots) {
            if (other.buildingId == selectedRootId) continue;
            AABB moving = rootBoundsAt(*root, V3(sx, p[1], sz));
            V3 otherPosition = other.position;
            AABB obstacle = rootBoundsAt(other, otherPosition);
            if (!volumeOverlap(moving, obstacle)) continue;

            struct Candidate { float value; bool xAxis; float distance; };
            std::vector<Candidate> candidates;
            auto addX = [&](float value) {
                if (value >= minX-1e-4f && value <= maxX+1e-4f)
                    candidates.push_back({value, true, std::fabs(value-requestedX)});
            };
            auto addZ = [&](float value) {
                if (value >= minZ-1e-4f && value <= maxZ+1e-4f)
                    candidates.push_back({value, false, std::fabs(value-requestedZ)});
            };
            const float left = sx + obstacle.minC[0] - moving.maxC[0];
            const float right = sx + obstacle.maxC[0] - moving.minC[0];
            const float back = sz + obstacle.minC[2] - moving.maxC[2];
            const float front = sz + obstacle.maxC[2] - moving.minC[2];

            // When the old box identifies an approach side, do not tunnel
            // through the obstacle merely because its far face is now nearer.
            if (oldBounds.maxC[0] <= obstacle.minC[0]+1e-4f) addX(left);
            if (oldBounds.minC[0] >= obstacle.maxC[0]-1e-4f) addX(right);
            if (oldBounds.maxC[2] <= obstacle.minC[2]+1e-4f) addZ(back);
            if (oldBounds.minC[2] >= obstacle.maxC[2]-1e-4f) addZ(front);
            if (candidates.empty()) {
                addX(left); addX(right); addZ(back); addZ(front);
            }
            if (candidates.empty()) return rootPosition;
            const Candidate best = *std::min_element(candidates.begin(), candidates.end(),
                [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
            if (best.xAxis) sx = best.value; else sz = best.value;
            corrected = true;
            break;
        }
        if (!corrected) return V3(sx, p[1], sz);
    }

    // A crowded arrangement could make iterative corrections oscillate. Keep
    // the last valid placement instead of committing a volume intersection.
    for (const ShapeNode& other : engine.roots) {
        if (other.buildingId == selectedRootId) continue;
        V3 otherPosition = other.position;
        if (volumeOverlap(rootBoundsAt(*root, V3(sx,p[1],sz)),
                          rootBoundsAt(other, otherPosition))) return rootPosition;
    }

    return V3(sx, p[1], sz);
}

void SceneController::moveSelectedTo(const V3& position) {
    if (selectedRootId < 0) return;
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return;
    V3 current = root->position;
    V3 next = position;
    if (current[0] == next[0] && current[1] == next[1] && current[2] == next[2]) return;

    engine.MoveRoot(selectedRootId, position);
    // Joining can add/remove temporary split-wall children, so the selected
    // building's derivation tree and child-index paths may have changed.
    highlightActive = false;
    highlightPath.clear();
    if (selectionChanged) selectionChanged();
    notifyChanged();
}

} // namespace sg
