#pragma once

#include "grammar/ShapeNode.h"

namespace building_rules {

struct Parameters {
    int floors = 3;       // Includes the ground floor.
    int bays = 3;         // Repeated facade sections per upper floor.
    float width = 6.0f;
    float floorHeight = 1.5f;
    float depth = 3.0f;
    bool balconies = true; // Explicit apartment alternative, never random.
};

// Building recipes only: return local ShapeNode trees, not rendered geometry.
// Root dimensions/position use world units; descendants use parent fractions.
// The engine implements Operations.h and resolves these trees to world-space leaves.
ShapeNode makeOffice(const Parameters& parameters);
ShapeNode makeApartment(const Parameters& parameters);

} // namespace building_rules
