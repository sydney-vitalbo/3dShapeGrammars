
#pragma once

#include <vector>
#include <ostream>

//using namespace std;

#include "ShapeNode.h"
#include "Rules.h"
#include "V3.h"

class GrammarEngine {
public:

    Rules rules;
    std::vector<ShapeNode> terminals;

    GrammarEngine();
    ~GrammarEngine();

    void GenerateFacade();
    void Expand(ShapeNode node);
};
