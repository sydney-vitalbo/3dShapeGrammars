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

// Partition the parent's full extent along axis, from its negative to positive
// edge. Fixed lengths use absolute units; relative weights share the remainder.
// Return childless nonterminal scopes with absolute dimensions, parent-local
// centers, and inherited building context. Other dimensions remain unchanged.
// All-fixed splits must fill the extent (allowing floating-point roundoff).
// Invalid dimensions, axes, or splits throw std::invalid_argument.
std::vector<ShapeNode> subdivide(const ShapeNode& parent, Axis axis,
                               const std::vector<Split>& splits);

// Fit count equal nonterminal scopes along axis, with absolute dimensions and
// parent-local centers. Gap is an absolute distance between children, with no
// end gaps. Building context is inherited; the parent remains unchanged.
// Invalid counts, gaps, dimensions, or axes throw std::invalid_argument.
std::vector<ShapeNode> repeat(const ShapeNode& parent, Axis axis, int count,
                            float gap = 0.0f);

// Append a rigid transform in the node's local axes: translate using its current
// orientation, then compose the supplied rotation. Rotation leaves the node's
// center fixed. Dimensions and child-local poses stay unchanged.
void transform(ShapeNode& node, V3 translation, M33 rotation = M33(1.0f));

// Return the supplied parent as a nonterminal with the supplied children.
// Preserve its position, dimensions, symbol, and material; replace its children.
ShapeNode compose(ShapeNode parent, std::vector<ShapeNode> children);

// Return children, either nonterminal or slab
ShapeNode childOf(const ShapeNode& parent, Symbol symbol, V3 size,
                  V3 position = V3(0, 0, 0));
ShapeNode terminal(const ShapeNode& parent, Material material, V3 size,
                   V3 position = V3(0, 0, 0));

} // namespace grammar_ops
