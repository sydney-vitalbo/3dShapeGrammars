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
