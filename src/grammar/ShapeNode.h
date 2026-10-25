#pragma once
#include "V3.h"
#include <vector>

enum class Symbol {
    Facade, Window, Slab,
};

enum class Material {
    None, Wood, Glass, Stone,
};

struct ShapeNode {
    int id = -1;
    Symbol symbol = Symbol::Slab;
    Material material = Material::None;

    V3 position = V3(0, 0, 0); // Center relative to parent
    V3 size = V3(1, 1, 1);

    bool isTerminal = false;
    std::vector<ShapeNode> children;
};