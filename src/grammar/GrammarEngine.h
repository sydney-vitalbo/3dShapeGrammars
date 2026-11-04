
#pragma once

#include <vector>
#include <ostream>

//using namespace std;

#include "ShapeNode.h"
#include "BuildingRules.h"
#include "V3.h"

class GrammarEngine {
public:

    std::vector<ShapeNode> terminals;
    // All object trees in the scene. Use AddRoot/ReplaceRoot/MoveRoot to keep
    // the combined world-space terminal cache synchronized with these trees.
    std::vector<ShapeNode> roots;

    GrammarEngine();
    ~GrammarEngine();

    // Store already-expanded trees. Root IDs stay stable across vector growth.
    int AddRoot(ShapeNode root);
    void ReplaceRoot(int buildingId, ShapeNode root);
    void MoveRoot(int buildingId, V3 position);
    const ShapeNode* FindRoot(int buildingId) const;
    void RebuildTerminals();
    // Preview a tree without changing the scene's terminal cache.
    std::vector<ShapeNode> TerminalsForRoot(const ShapeNode& root) const;

    void GenerateTest();
    void GenerateTree(ShapeNode* node);
    void PrintTree(const ShapeNode* node, int depth = 0);
    void CollectTerminals(ShapeNode* node, V3 parentPose, M33 parentRotation = M33(1.0f));
    void PrintTerminals();

private:
    int nextBuildingId = 0;
    static void AssignBuildingId(ShapeNode& node, int buildingId);
    static void CollectInto(const ShapeNode* node, V3 parentPose, M33 parentRotation,
                            int buildingId, std::vector<ShapeNode>& output);
};
