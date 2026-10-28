#pragma once

#include "grammar/ShapeNode.h"
#include <vector>

// Interface required by BuildingRules.cpp. Mridu's engine supplies the four
// operation implementations. This header contains no expansion or rendering.
namespace grammar_ops {

enum class Axis { X = 0, Y = 1, Z = 2 };

struct Split {
    enum class Kind { Fixed, Relative };
    Kind kind;
    float value;

    // Value constructors only: these describe a split, they do not perform it.
    static Split fixed(float length) { return {Kind::Fixed, length}; }
    static Split relative(float weight) { return {Kind::Relative, weight}; }
};

// Fixed lengths use the units of extent; relative weights share the remainder.
// Return nonterminal child scopes with parent-relative positions and sizes.
// Scopes fill [-0.5, +0.5] along axis; other axes retain center 0 and size 1.
std::vector<ShapeNode> subdivide(Axis axis, float extent,
                               const std::vector<Split>& splits);

// Fit count equal nonterminal scopes inside the parent along axis.
// Gap is a fraction of the parent's extent, between children, with no end gaps.
std::vector<ShapeNode> repeat(Axis axis, int count, float gap = 0.0f);

// Return the supplied parent as a nonterminal with the supplied children.
// Preserve its position, dimensions, symbol, and material; replace its children.
ShapeNode compose(ShapeNode parent, std::vector<ShapeNode> children);

// Return a childless terminal Slab with these parent-relative dimensions/center.
ShapeNode terminal(Material material, const V3& size = V3(1, 1, 1),
                   const V3& position = V3(0, 0, 0));

} // namespace grammar_ops
