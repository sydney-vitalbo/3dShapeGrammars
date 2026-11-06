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

// used by remove root UI button
bool GrammarEngine::RemoveRoot(int buildingId) {
    const auto root = std::find_if(roots.begin(), roots.end(),
        [buildingId](const ShapeNode& node) { return node.buildingId == buildingId; });
    if (root == roots.end()) return false;
    roots.erase(root);
    RebuildTerminals();
    return true;
}

// regenerate root after snapping to new position, used by SceneController::setBuilding()/snapToGrid()
void GrammarEngine::MoveRoot(int buildingId, V3 position) {
    for (ShapeNode& root : roots) {
        if (root.buildingId != buildingId) continue;
        V3 previous = root.position;
        if (previous[0] == position[0] && previous[1] == position[1] && previous[2] == position[2]) return;
        root.position = position;
        // rebuild terminals with potential shared wall removal
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

// remove shared wall and re-collect terminals for all buildings
void GrammarEngine::RebuildTerminals() {
    UpdateJoins();
    std::vector<ShapeNode> result;
    for (const ShapeNode& root : roots)
        CollectInto(&root, V3(0, 0, 0), M33(1.0f), root.buildingId, result);
    terminals.swap(result);
}

// assess joins between any pair of building roots using AABBs
void GrammarEngine::UpdateJoins() {
    std::vector<unsigned> masks(roots.size(), 0);
    for (ShapeNode& root : roots) {
        if (!root.rootJoin) root.rootJoin.emplace();
        root.rootJoin->bounds = rootBounds(root);
    }
    std::vector<std::vector<SharedWallSpan>> spans(roots.size());
    for (size_t i = 0; i < roots.size(); ++i) {
        for (size_t j = i+1; j < roots.size(); ++j) {
            AABB a = roots[i].rootJoin->bounds, b = roots[j].rootJoin->bounds;
            // A completely contained root contributes no exterior. Identical
            // boxes keep the older root visible instead of hiding both copies.
            if (contains(a,b)) { masks[j] |= allBuildingFaces; continue; }
            if (contains(b,a)) { masks[i] |= allBuildingFaces; continue; }
            const unsigned ab = coveredFaces(a,b), ba = coveredFaces(b,a);
            masks[i] |= ab;
            masks[j] |= ba;
            {
                for (unsigned face : {1u,2u,16u,32u}) {
                    const int tangent = face <= 2 ? 2 : 0;
                    const float lo = std::max(a.minC[tangent], b.minC[tangent]);
                    const float hi = std::min(a.maxC[tangent], b.maxC[tangent]);
                    if (ab & face) spans[i].push_back({face,lo,hi,std::max(a.minC[1],b.minC[1]),std::min(a.maxC[1],b.maxC[1])});
                    if (ba & face) spans[j].push_back({face,lo,hi,std::max(a.minC[1],b.minC[1]),std::min(a.maxC[1],b.maxC[1])});
                }
            }
        }
    }
    for (size_t i = 0; i < roots.size(); ++i) {
        RootJoinState& state = *roots[i].rootJoin;
        // The covered interval can change even while the face mask stays the same.
        state.wallSpans = std::move(spans[i]);
        ApplyJoinMask(roots[i], masks[i], roots[i].rotation, state.wallSpans,
                      V3(0,0,0), M33(1.0f));
        state.lastRotation = roots[i].rotation;
        state.visibilityDirty = false;
        state.overlapMask = masks[i];
    }
}

void GrammarEngine::ApplyJoinMask(ShapeNode& node, unsigned worldMask, M33 rootRotation,
                                   const std::vector<SharedWallSpan>& spans,
                                   V3 parentPose, M33 parentRotation, bool parentDisabled) {
    const V3 worldPosition = parentPose + parentRotation * node.position;
    M33 worldRotation = parentRotation * node.rotation;
    const unsigned face = worldFace(node.exteriorFace, rootRotation);
    const bool faceCovered = (worldMask & face) != 0;
    // Restore the original slab first, so moves never accumulate subdivisions.
    if (node.joinSplit) {
        node.children.clear();
        node.isTerminal = true;
        node.joinSplit = false;
    }
    const bool frontBack = node.exteriorFace == BuildingFace::Front || node.exteriorFace == BuildingFace::Back;
    const bool side = node.exteriorFace == BuildingFace::Left || node.exteriorFace == BuildingFace::Right;
    if (!parentDisabled && worldMask != allBuildingFaces && faceCovered &&
        (frontBack || side) && node.symbol == Symbol::Slab && node.isTerminal) {
        const int axis = frontBack ? 0 : 2;
        const float length = node.size[axis];
        V3 tangent(0,0,0); tangent[axis] = 1;
        tangent = worldRotation * tangent;
        const int worldAxis = face <= 2 ? 2 : 0;
        struct Rectangle { float min, max, minY, maxY; };
        std::vector<Rectangle> covered;
        const float height = node.size[1];
        V3 center = worldPosition;
        // Exact interval clipping applies to axis-aligned/quarter-turn walls.
        V3 up = worldRotation * V3(0,1,0);
        const bool aligned = std::fabs(std::fabs(tangent[worldAxis])-1.f) < joinEpsilon &&
                             std::fabs(up[1]-1.f) < joinEpsilon;
        if (aligned) {
            for (const auto& span : spans) if (span.face == face) {
                float lo = (span.min-center[worldAxis])/tangent[worldAxis];
                float hi = (span.max-center[worldAxis])/tangent[worldAxis];
                if (lo > hi) std::swap(lo,hi);
                lo = std::max(lo,-length/2); hi = std::min(hi,length/2);
                const float bottom = std::max(span.minY-center[1],-height/2);
                const float top = std::min(span.maxY-center[1],height/2);
                if (hi-lo > joinEpsilon && top-bottom > joinEpsilon)
                    covered.push_back({lo,hi,bottom,top});
            }
        }
        // Keep oblique roots' previous conservative whole-face behaviour.
        if (!aligned) covered.push_back({-length/2,length/2,-height/2,height/2});
        if (covered.empty()) {
            // This slab is above/below the neighbour: retain the original box.
            node.disabled = false;
            return;
        }
        const float beam = frontBack ? 0.1f : 0.f;
        const float lo = -length/2+beam, hi = length/2-beam;
        std::vector<float> cuts{lo,hi}, heightCuts{-height/2,height/2};
        for (const auto& span : covered) {
            cuts.push_back(std::clamp(span.min,lo,hi));
            cuts.push_back(std::clamp(span.max,lo,hi));
            heightCuts.push_back(span.minY);
            heightCuts.push_back(span.maxY);
        }
        auto sortCuts = [](std::vector<float>& values) {
            std::sort(values.begin(),values.end());
            values.erase(std::unique(values.begin(),values.end(),[](float a,float b){
                return std::fabs(a-b)<joinEpsilon;
            }),values.end());
        };
        sortCuts(cuts);
        sortCuts(heightCuts);
        // Crossing width and height cuts yields four cells for a single offset
        // corner overlap; only cells inside a covered rectangle are hidden.
        for (size_t y=1; y<heightCuts.size(); ++y)
        for (size_t i=1; i<cuts.size(); ++i) {
            const float midpoint = (cuts[i-1]+cuts[i])/2;
            V3 size=node.size, position(0,0,0);
            size[axis]=cuts[i]-cuts[i-1]; position[axis]=midpoint;
            size[1]=heightCuts[y]-heightCuts[y-1];
            position[1]=(heightCuts[y]+heightCuts[y-1])/2;
            auto piece=grammar_ops::terminal(node,node.material,size,position);
            for (const auto& span : covered)
                if (midpoint >= span.min && midpoint <= span.max &&
                    position[1] >= span.minY && position[1] <= span.maxY) piece.disabled=true;
            node.children.push_back(std::move(piece));
        }
        if (frontBack) for (float sign : {-1.f,1.f}) {
            const float z=(node.exteriorFace==BuildingFace::Front ? 1.f : -1.f)*(node.size[2]-beam)/2;
            auto edge=grammar_ops::terminal(node,node.material,V3(beam,node.size[1],beam),
                                           V3(sign*(length-beam)/2,0,z));
            node.children.push_back(std::move(edge));
        }
        node.isTerminal=false;
        node.joinSplit=true;
    }
    node.disabled = parentDisabled || worldMask == allBuildingFaces || (faceCovered && !node.joinSplit);
    if (!node.joinSplit) for (ShapeNode& child : node.children)
        ApplyJoinMask(child,worldMask,rootRotation,spans,worldPosition,worldRotation,node.disabled);
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
