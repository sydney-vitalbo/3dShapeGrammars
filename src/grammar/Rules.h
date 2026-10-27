
#pragma once

#include "ShapeNode.h"

class Rules {
public:
    Rules() {};
    void Expand(ShapeNode* node);
    void Window(ShapeNode* parent);
    void Facade(ShapeNode* parent);
};