#include "grammar/Operations.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace grammar_ops {

std::vector<ShapeNode> subdivide(const ShapeNode& parent, Axis axis,
                               const std::vector<Split>& splits) {
    const int axisIndex = static_cast<int>(axis);
    if (axisIndex < 0 || axisIndex > 2 || splits.empty())
        throw std::invalid_argument("Subdivision needs a valid axis and at least one split.");

    V3 parentSize = parent.size;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(parentSize[i]) || parentSize[i] <= 0)
            throw std::invalid_argument("Subdivision needs finite, positive parent dimensions.");
    }

    const double extent = parentSize[axisIndex];
    double fixedTotal = 0, weightTotal = 0;
    for (const Split& split : splits) {
        if (!std::isfinite(split.value) || split.value <= 0)
            throw std::invalid_argument("Split lengths and relative weights must be finite and positive.");
        switch (split.kind) {
        case Split::Kind::Fixed:    fixedTotal += split.value; break;
        case Split::Kind::Relative: weightTotal += split.value; break;
        default: throw std::invalid_argument("Unknown split kind.");
        }
    }

    const double remaining = extent - fixedTotal;
    if (weightTotal > 0) {
        if (remaining <= 0)
            throw std::invalid_argument("Fixed splits must leave positive space for relative splits.");
    } else if (std::fabs(remaining) > 1e-5 * extent) {
        throw std::invalid_argument("All-fixed splits must fill the parent extent.");
    }

    std::vector<ShapeNode> children;
    children.reserve(splits.size());
    double cursor = -extent / 2;
    for (size_t i = 0; i < splits.size(); ++i) {
        const Split& split = splits[i];
        double length = split.kind == Split::Kind::Fixed ? split.value
            : remaining * split.value / weightTotal;
        // Absorb roundoff at the last boundary, keeping the partition closed.
        if (i + 1 == splits.size())
            length = extent / 2 - cursor;
        if (!std::isfinite(length) || static_cast<float>(length) <= 0)
            throw std::invalid_argument("Subdivision produced a nonpositive or unrepresentable length.");

        V3 size = parentSize;
        V3 position(0, 0, 0);
        size[axisIndex] = static_cast<float>(length);
        position[axisIndex] = static_cast<float>(cursor + length / 2);
        children.push_back(childOf(parent, Symbol::Slab, size, position));
        cursor += length;
    }
    return children;
}

std::vector<ShapeNode> repeat(const ShapeNode& parent, Axis axis, int count, float gap) {
    if (count < 1 || !std::isfinite(gap) || gap < 0)
        throw std::invalid_argument("Repetition needs a positive count and a finite, nonnegative gap.");

    if (gap == 0 || count == 1)
        return subdivide(parent, axis, std::vector<Split>(count, Split::relative(1)));

    // Equal weighted regions separated by fixed gaps. subdivide validates that
    // the gaps leave positive space, and calculates all local centers/sizes.
    std::vector<Split> splits;
    splits.reserve(static_cast<size_t>(count) * 2 - 1);
    for (int i = 0; i < count; ++i) {
        if (i > 0)
            splits.push_back(Split::fixed(gap));
        splits.push_back(Split::relative(1));
    }
    auto regions = subdivide(parent, axis, splits);
    std::vector<ShapeNode> children;
    children.reserve(count);
    for (size_t i = 0; i < regions.size(); i += 2)
        children.push_back(std::move(regions[i])); // Gaps produce no nodes.
    return children;
}

void transform(ShapeNode& node, V3 translation, M33 rotation) {
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(translation[i]))
            throw std::invalid_argument("Transform translation must be finite.");
        for (int j = 0; j < 3; ++j) {
            if (!std::isfinite(rotation[i][j]))
                throw std::invalid_argument("Transform rotation must be finite.");
        }
    }
    node.position = node.position + node.rotation * translation;
    node.rotation = node.rotation * rotation;
}

ShapeNode childOf(const ShapeNode& parent, Symbol symbol, V3 size,
                  V3 position) {
    ShapeNode child;
    child.symbol = symbol;
    child.size = size;
    child.position = position;
    child.building = parent.building;
    return child;
}

ShapeNode terminal(const ShapeNode& parent, Material material, V3 size,
                V3 position) {
    ShapeNode child = childOf(parent, Symbol::Slab, size, position);
    child.isTerminal = true;
    child.material = material;
    return child;
}

}
