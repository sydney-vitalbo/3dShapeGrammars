#pragma once
#include "V3.h"
#include <vector>

enum class Symbol {
    Facade, Window, Slab,
    Office, Apartment, Floor, WallRegion, BayGroup, Bay, Entrance, Balcony, Roof,
};

enum class Material {
    None, Wood, Glass, Stone,
};

// Context inherited by a building rule's children for deferred expansion.
struct BuildingContext {
    int floors = 3;
    int bays = 3;
    bool apartment = false;
    bool groundFloor = false;
    bool balconies = true;
    float front = 1.0f; // +Z street facade or -Z rear facade.
};

struct ShapeNode {
    int id = -1;
    Symbol symbol = Symbol::Slab;
    Material material = Material::None;

    V3 position = V3(0, 0, 0); // Center relative to parent
    V3 size = V3(1, 1, 1); // Absolute dimensions; never multiplied by parent size.

    BuildingContext building;

    bool isTerminal = false;
    std::vector<ShapeNode> children;
};
