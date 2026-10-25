
#pragma once

#include "ShapeNode.h"

class Rules {
public:
    Rules() {};
    std::vector<ShapeNode> Expand(const ShapeNode node);
    std::vector<ShapeNode> Window(const ShapeNode parent);
    std::vector<ShapeNode> Facade(const ShapeNode parent);
};