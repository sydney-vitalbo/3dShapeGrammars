#include "GrammarEngine.h"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cmath>

namespace {
constexpr float joinEpsilon = 1e-4f;

AABB rootBounds(const ShapeNode& root) {
    AABB bounds;
    V3 size = root.size, position = root.position;
    M33 rotation = root.rotation;
    for (int x : {-1, 1}) for (int y : {-1, 1}) for (int z : {-1, 1})
        bounds.AddPoint(position + rotation * V3(x*size[0]/2, y*size[1]/2, z*size[2]/2));
    return bounds;
}

bool contains(AABB outer, AABB inner) {
    for (int axis = 0; axis < 3; ++axis)
        if (inner.minC[axis] < outer.minC[axis]-joinEpsilon ||
            inner.maxC[axis] > outer.maxC[axis]+joinEpsilon) return false;
    return true;
}

unsigned coveredFaces(AABB a, AABB b) {
    unsigned mask = 0;
    const unsigned positive[] = {1, 4, 16}, negative[] = {2, 8, 32};
    for (int axis = 0; axis < 3; ++axis) {
        bool area = true;
        for (int other = 0; other < 3; ++other) {
            if (other == axis) continue;
            if (std::min(a.maxC[other], b.maxC[other]) -
                std::max(a.minC[other], b.minC[other]) <= joinEpsilon) area = false;
        }
        if (!area) continue; // Edge/corner-only contact is not a shared wall.
        // The neighbour must be on the outward side. Coincident roofs/bases
        // with equal outward normals remain exterior during sideways overlap.
        if (b.minC[axis] <= a.maxC[axis]+joinEpsilon &&
            b.maxC[axis] >= a.maxC[axis]-joinEpsilon &&
            (b.maxC[axis] > a.maxC[axis]+joinEpsilon ||
             std::fabs(b.minC[axis]-a.maxC[axis]) <= joinEpsilon)) mask |= positive[axis];
        if (b.minC[axis] <= a.minC[axis]+joinEpsilon &&
            b.maxC[axis] >= a.minC[axis]-joinEpsilon &&
            (b.minC[axis] < a.minC[axis]-joinEpsilon ||
             std::fabs(b.maxC[axis]-a.minC[axis]) <= joinEpsilon)) mask |= negative[axis];
    }
    return mask;
}

unsigned worldFace(BuildingFace face, M33 rotation) {
    V3 normal(0, 0, 0);
    switch (face) {
    case BuildingFace::Right:  normal = V3(1,0,0); break;
    case BuildingFace::Left:   normal = V3(-1,0,0); break;
    case BuildingFace::Top:    normal = V3(0,1,0); break;
    case BuildingFace::Bottom: normal = V3(0,-1,0); break;
    case BuildingFace::Front:  normal = V3(0,0,1); break;
    case BuildingFace::Back:   normal = V3(0,0,-1); break;
    default: return 0;
    }
    normal = rotation * normal;
    int axis = 0;
    for (int i = 1; i < 3; ++i)
        if (std::fabs(normal[i]) > std::fabs(normal[axis])) axis = i;
    const unsigned positive[] = {1, 4, 16}, negative[] = {2, 8, 32};
    return normal[axis] >= 0 ? positive[axis] : negative[axis];
}
} // namespace

GrammarEngine::GrammarEngine() {
    terminals = std::vector<ShapeNode>();
    roots = std::vector<ShapeNode>();
}

GrammarEngine::~GrammarEngine() {
    terminals.clear();
}

// Expand the facade rule, build tree,
// append terminal shape nodes to terminals vector
void GrammarEngine::GenerateTest() {
    ShapeNode root;
    root.symbol = Symbol::Facade;
    root.position = V3(0, 0, -3.0f);
    root.size = V3(4.0f, 4.0f, 0.2f);
    root.isTerminal = false;
    root.id = 0;

    // override with apartment node for testing
    //building_rules::Parameters params;
    // testing subdivison (splitting height into floors) and repetition (bays within floor)
    //params.floors = 3;
    //params.bays = 3;
    //root = building_rules::buildingRoot(params, false);

    // Rules determine all bays, windows, and terminal geometry.
    GenerateTree(&root);
    AddRoot(std::move(root));
}

void GrammarEngine::AssignBuildingId(ShapeNode& node, int buildingId) {
    node.buildingId = buildingId;
    for (ShapeNode& child : node.children)
        AssignBuildingId(child, buildingId);
}

int GrammarEngine::AddRoot(ShapeNode root) {
    const int buildingId = nextBuildingId++;
    AssignBuildingId(root, buildingId);
    root.rootJoin.reset();
    roots.push_back(std::move(root));
    RebuildTerminals();
    return buildingId;
}

const ShapeNode* GrammarEngine::FindRoot(int buildingId) const {
    for (const ShapeNode& root : roots)
        if (root.buildingId == buildingId) return &root;
    return nullptr;
}

void GrammarEngine::ReplaceRoot(int buildingId, ShapeNode root) {
    for (ShapeNode& current : roots) {
        if (current.buildingId != buildingId) continue;
        AssignBuildingId(root, buildingId);
        root.rootJoin.reset();
        current = std::move(root);
        RebuildTerminals();
        return;
    }
    throw std::invalid_argument("Building no longer exists.");
}

void GrammarEngine::MoveRoot(int buildingId, V3 position) {
    for (ShapeNode& root : roots) {
        if (root.buildingId != buildingId) continue;
        V3 previous = root.position;
        if (previous[0] == position[0] && previous[1] == position[1] && previous[2] == position[2]) return;
        root.position = position;
        RebuildTerminals();
        return;
    }
    throw std::invalid_argument("Building no longer exists.");
}

std::vector<ShapeNode> GrammarEngine::TerminalsForRoot(const ShapeNode& root) const {
    std::vector<ShapeNode> result;
    CollectInto(&root, V3(0, 0, 0), M33(1.0f), root.buildingId, result, true);
    return result;
}

void GrammarEngine::RebuildTerminals() {
    UpdateJoins();
    std::vector<ShapeNode> result;
    for (const ShapeNode& root : roots)
        CollectInto(&root, V3(0, 0, 0), M33(1.0f), root.buildingId, result);
    terminals.swap(result);
}

void GrammarEngine::UpdateJoins() {
    std::vector<unsigned> masks(roots.size(), 0);
    for (ShapeNode& root : roots) {
        if (!root.rootJoin) root.rootJoin.emplace();
        root.rootJoin->bounds = rootBounds(root);
    }
    for (size_t i = 0; i < roots.size(); ++i) {
        for (size_t j = i+1; j < roots.size(); ++j) {
            AABB a = roots[i].rootJoin->bounds, b = roots[j].rootJoin->bounds;
            // A completely contained root contributes no exterior. Identical
            // boxes keep the older root visible instead of hiding both copies.
            if (contains(a,b)) { masks[j] |= allBuildingFaces; continue; }
            if (contains(b,a)) { masks[i] |= allBuildingFaces; continue; }
            masks[i] |= coveredFaces(a,b);
            masks[j] |= coveredFaces(b,a);
        }
    }
    for (size_t i = 0; i < roots.size(); ++i) {
        RootJoinState& state = *roots[i].rootJoin;
        bool orientationChanged = false;
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c)
            if (state.lastRotation[r][c] != roots[i].rotation[r][c]) orientationChanged = true;
        if (state.overlapMask != masks[i] || state.visibilityDirty || orientationChanged) {
            // Reapply from scratch so separation restores the affected tree.
            ApplyJoinMask(roots[i], masks[i], roots[i].rotation);
            state.lastRotation = roots[i].rotation;
            state.visibilityDirty = false;
        }
        state.overlapMask = masks[i];
    }
}

void GrammarEngine::ApplyJoinMask(ShapeNode& node, unsigned worldMask, M33 rootRotation,
                                   bool parentDisabled) {
    node.disabled = parentDisabled || worldMask == allBuildingFaces ||
        (worldMask & worldFace(node.exteriorFace, rootRotation)) != 0;
    for (ShapeNode& child : node.children)
        ApplyJoinMask(child, worldMask, rootRotation, node.disabled);
}

// generate grammar tree for a given preset
void GrammarEngine::GenerateTree(ShapeNode* node) {
    if (node == nullptr) {
        std::cerr << "Error: GenerateTree called with null node." << std::endl;
        return;
    }
    if (node->isTerminal) {
        return;
    }
    if (node->rootJoin) node->rootJoin->visibilityDirty = true;

    // generate grammar subtree for this node
    building_rules::Expand(node);

    // recursively generate grammar tree for each child
    for (ShapeNode& child : node->children) {
        GenerateTree(&child);
    }
}

// collect the terminal nodes with their world pose
void GrammarEngine::CollectTerminals(ShapeNode* node, V3 parentPose, M33 parentRotation) {
    CollectInto(node, parentPose, parentRotation, node ? node->buildingId : -1, terminals);
}

void GrammarEngine::CollectInto(const ShapeNode* node, V3 parentPose, M33 parentRotation,
                                int buildingId, std::vector<ShapeNode>& output, bool includeDisabled) {
    if (node == nullptr) {
        std::cerr << "Error: CollectTerminals called with null node." << std::endl;
        return;
    }
    if (node->disabled && !includeDisabled) return;

    V3 newParentPose = parentPose + parentRotation * node->position;
    M33 worldRotation = parentRotation * node->rotation;

    // If the node is terminal, add it to the terminals vector
    if (node->isTerminal) {
        // copy and transform to world pose first
        ShapeNode terminal = *node;
        terminal.position = newParentPose;
        terminal.rotation = worldRotation;
        terminal.buildingId = buildingId;
        terminal.rootJoin.reset(); // Spatial state belongs only to scene roots.
        output.push_back(std::move(terminal));
        return;
    }

    // If the node is not terminal, recursively collect terminals from its children
    for (const ShapeNode& child : node->children) {
        CollectInto(&child, newParentPose, worldRotation, buildingId, output, includeDisabled);
    }
}

// print the grammar tree for debugging
void GrammarEngine::PrintTree(const ShapeNode* node, int depth) {
    if (node == nullptr) {
        std::cerr << "Error: PrintTree called with null node." << std::endl;
        return;
    }

    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }
    std::cout << int(node->symbol) << " (ID: " << node->id << ")" << " (Position: " << node->position << ")" << " (Size: " << node->size << ")" << std::endl;

    for (const ShapeNode& child : node->children) {
        PrintTree(&child, depth + 1);
    }
}

// print the collected terminal nodes for debugging
void GrammarEngine::PrintTerminals() {
    std::cout << "Collected Terminal Nodes:" << std::endl;
    for (const ShapeNode& terminal : terminals) {
        std::cout << "ID: " << terminal.id
                  << ", Symbol: " << int(terminal.symbol)
                  << ", Material: " << int(terminal.material)
                  << ", Position: " << terminal.position
                  << ", Size: " << terminal.size
                  << std::endl;
    }
}
