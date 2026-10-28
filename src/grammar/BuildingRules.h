#pragma once

#include "grammar/ShapeNode.h"
#include "grammar/Operations.h"

namespace building_rules {

struct Parameters {
    int floors = 1;       // Includes the ground floor.
    int bays = 2;         // Repeated facade sections per upper floor.
    float width = 4.0f;
    float floorHeight = 1.5f;
    float depth = 2.0f;
    bool balconies = true; // Explicit apartment alternative, never random.
};

constexpr float bandHeight = 0.12f;
constexpr float roofHeight = 0.24f;

// Helpers
void validate(const Parameters& p);
ShapeNode buildingRoot(const Parameters& p, bool apartment);

// Create an unexpanded root only. GrammarEngine::GenerateTree expands it.
// All dimensions use absolute units; tree positions are relative to the parent.
ShapeNode makeOffice(const Parameters& parameters);
ShapeNode makeApartment(const Parameters& parameters);

// Each production replaces only this node's immediate children. No production
// calls another production; recursion belongs to GrammarEngine::GenerateTree.
void expandBuilding(ShapeNode* node);
void expandFloor(ShapeNode* node);
void expandWallRegion(ShapeNode* node);
void expandFacade(ShapeNode* node);
void expandBayGroup(ShapeNode* node);
void expandBay(ShapeNode* node);
void expandEntrance(ShapeNode* node);
void expandWindow(ShapeNode* node);
void expandBalcony(ShapeNode* node);
void expandRoof(ShapeNode* node);

} // namespace building_rules
