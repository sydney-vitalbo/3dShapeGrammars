#pragma once
#include "V3.h"
#include "M33.h"
#include <vector>
#include <optional>

enum class Symbol {
    Facade, Window, Slab,
    Office, Apartment, Floor, WallRegion, BayGroup, Bay, Entrance, Balcony, Roof, JoinedUnit,
};

enum class Material {
    None, Wood, Glass, Stone,
};

// Root masks use world AABB directions. Component tags use building-local axes.
enum class BuildingFace : unsigned {
    None = 0, Right = 1, Left = 2, Top = 4, Bottom = 8, Front = 16, Back = 32
};
constexpr unsigned allBuildingFaces = 63;

struct SharedWallSpan {
    unsigned face; // World face bit; interval runs along X for front/back, Z for sides.
    float min, max;
    float minY, maxY; // World-space vertical extent of the covered rectangle.
    float roofMinY; // Neighbour's roof starts here; below it are floor slabs.
};

struct RootJoinState {
    AABB bounds; // Structural root box in world space, excluding decorations.
    unsigned overlapMask = 0;
    std::vector<SharedWallSpan> wallSpans; // Neighbours' covered wall rectangles.
    M33 lastRotation = M33(1.0f);
    bool visibilityDirty = true;
};

// Context inherited by a building rule's children for deferred expansion.
struct BuildingContext {
    int floors = 1;
    int bays = 1;
    bool apartment = false;
    bool groundFloor = false;
    bool balconies = true;
    bool streetFacing = true; // Design choice; orientation is stored in the pose.
};

struct ShapeNode {
    int id = -1;
    int buildingId = -1; // Stable building/unit ID; collected copies use the selectable root ID.
    Symbol symbol = Symbol::Slab;
    Material material = Material::None;

    V3 position = V3(0, 0, 0); // Center relative to parent
    M33 rotation = M33(1.0f); // Local orientation; world orientation in collected copies.
    V3 size = V3(1, 1, 1); // Absolute dimensions; never multiplied by parent size.

    BuildingContext building;

    // Populated for building roots and JoinedUnit roots, never grammar components.
    std::optional<RootJoinState> rootJoin;
    BuildingFace exteriorFace = BuildingFace::None;
    bool disabled = false; // Derived from root joining; reset on separation.
    bool joinSplit = false; // Temporary shared/exposed slab split; restored on separation.
    // Former unit members that this building must not immediately rejoin.
    // Entries are removed independently as those two buildings separate, so a
    // detached building may still join a different building right away.
    std::vector<int> detachedFromBuildings;
    bool lockWidth = false, lockDepth = false; // JoinedUnit parameter constraints.

    bool isTerminal = false;
    std::vector<ShapeNode> children;

    // Cached AABB- the engine fills this in once per terminal rebuild so dont have to recompute on every click/drag
    // **only meaningful for terminals since already in world space coordinates!
    // (left at the default (empty) box on grammar-tree nodes, which are expressed in parent-relative coordinates)
    AABB worldAABB;
};
