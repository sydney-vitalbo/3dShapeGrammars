
#pragma once

#include <vector>
#include <ostream>

//using namespace std;

#include "ShapeNode.h"
#include "Rules.h"
#include "BuildingRules.h"
#include "V3.h"

class GrammarEngine {
public:

    Rules rules;
    std::vector<ShapeNode> terminals;
    ShapeNode root;

    GrammarEngine();
    ~GrammarEngine();

    void GenerateTest();
    void GenerateTree(ShapeNode* node);
    void PrintTree(const ShapeNode* node, int depth = 0);
    void CollectTerminals(ShapeNode* node, V3 parentPose, M33 parentRotation = M33(1.0f));
    void PrintTerminals();
};
