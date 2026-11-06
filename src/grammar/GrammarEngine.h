
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
    // Removing a root also restores any walls exposed on its neighbours.
    bool RemoveRoot(int buildingId);
    const ShapeNode* FindRoot(int buildingId) const;
    // Refresh root bounds/contact masks, propagate visibility, then collect.
    void RebuildTerminals();
    // Preview full geometry, including disabled nodes, for stable grid bounds.
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
                            int buildingId, std::vector<ShapeNode>& output,
                            bool includeDisabled = false);
    void UpdateJoins();
    static void ApplyJoinMask(ShapeNode& node, unsigned worldMask, M33 rootRotation,
                              const std::vector<SharedWallSpan>& spans,
                              V3 parentPose, M33 parentRotation, bool parentDisabled = false,
                              unsigned facadeFace = 0, bool inRoof = false);
};
