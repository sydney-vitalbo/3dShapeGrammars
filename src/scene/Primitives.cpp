//Primitives.cpp
//Sydney Vitalbo


#include "scene/Primitives.h"

#include <cmath>

namespace sg {
namespace primitives {

Mesh makeUnitBox() {
    Mesh m;

    //1 face = 2 triangles that share an edge (1 quad = 2 triangles)
    auto addQuad = [&m](float ax, float ay, float az,
                        float bx, float by, float bz,
                        float cx, float cy, float cz,
                        float dx, float dy, float dz,
                        float nx, float ny, float nz) {
        unsigned int a = m.addVertex(ax, ay, az, nx, ny, nz);
        unsigned int b = m.addVertex(bx, by, bz, nx, ny, nz);
        unsigned int c = m.addVertex(cx, cy, cz, nx, ny, nz);
        unsigned int d = m.addVertex(dx, dy, dz, nx, ny, nz);
        m.addTriangle(a, b, c);
        m.addTriangle(a, c, d);
    };

    //unit box: edge length 1
    const float h = 0.5f; // half edge length

    //the 6 faces:
    // +X face (normal pointing +x)
    addQuad( h, -h, -h,   h,  h, -h,   h,  h,  h,   h, -h,  h,   1, 0, 0);
    // -X face
    addQuad(-h, -h,  h,  -h,  h,  h,  -h,  h, -h,  -h, -h, -h,  -1, 0, 0);
    // +Y face (top)
    addQuad(-h,  h, -h,   h,  h, -h,   h,  h,  h,  -h,  h,  h,   0, 1, 0);
    // -Y face (bottom)
    addQuad(-h, -h,  h,   h, -h,  h,   h, -h, -h,  -h, -h, -h,   0,-1, 0);
    // +Z face (front)
    addQuad(-h, -h,  h,   h, -h,  h,   h,  h,  h,  -h,  h,  h,   0, 0, 1);
    // -Z face (back)
    addQuad( h, -h, -h,  -h, -h, -h,  -h,  h, -h,   h,  h, -h,   0, 0,-1);

    return m;
}

Mesh makeUnitCylinder(int segments) {
    Mesh m;
    if (segments < 3) segments = 3; //min bound on this

    //strategy: approximate the circle with many short straight edges/segments

    const float r = 0.5f;          // radius (unit cylinder => diameter 1)
    const float yTop = 0.5f;       // top cap height   (so total height is 1)
    const float yBot = -0.5f;      // bottom cap height
    const float twoPi = 6.28318530717958647692f; // 2*pi = one full turn, radians

    //side wall: walk around the circle one segment at a time. Each segment has
    // two angles (a0 = start edge, a1 = next edge) and build the vertical strip
    // of wall between them as a quad (2 tri) from bottom to top
    for (int i = 0; i < segments; ++i) {
        float a0 = twoPi * (static_cast<float>(i) / segments);     // this edge's angle
        float a1 = twoPi * (static_cast<float>(i + 1) / segments); // next edge's angle

        //precomute cos and sin of each angle
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        //four corners of this wall segment (bottom/top left/right)
        //last 3 args are the outward normal (y = 0 always)
        unsigned int bl = m.addVertex(r * c0, yBot, r * s0, c0, 0, s0);
        unsigned int br = m.addVertex(r * c1, yBot, r * s1, c1, 0, s1);
        unsigned int tr = m.addVertex(r * c1, yTop, r * s1, c1, 0, s1);
        unsigned int tl = m.addVertex(r * c0, yTop, r * s0, c0, 0, s0);

        //split the quad (bl,br,tr,tl) into two triangles
        m.addTriangle(bl, br, tr);
        m.addTriangle(bl, tr, tl);
    }

    //top of cylinder: normal (0, 1, 0)
    unsigned int topCenter = m.addVertex(0, yTop, 0, 0, 1, 0);
    for (int i = 0; i < segments; ++i) {
        float a0 = twoPi * (static_cast<float>(i) / segments);
        float a1 = twoPi * (static_cast<float>(i + 1) / segments);
        unsigned int p0 = m.addVertex(r * std::cos(a0), yTop, r * std::sin(a0), 0, 1, 0);
        unsigned int p1 = m.addVertex(r * std::cos(a1), yTop, r * std::sin(a1), 0, 1, 0);
        m.addTriangle(topCenter, p0, p1);
    }

    //bottom of cylinder: normal (0, -1, 0)
    //list the two rim points in the OPPOSITE order (p1 before p0) of the top, so faces downwards
    unsigned int botCenter = m.addVertex(0, yBot, 0, 0, -1, 0);
    for (int i = 0; i < segments; ++i) {
        float a0 = twoPi * (static_cast<float>(i) / segments);
        float a1 = twoPi * (static_cast<float>(i + 1) / segments);
        unsigned int p0 = m.addVertex(r * std::cos(a0), yBot, r * std::sin(a0), 0, -1, 0);
        unsigned int p1 = m.addVertex(r * std::cos(a1), yBot, r * std::sin(a1), 0, -1, 0);
        m.addTriangle(botCenter, p1, p0);
    }

    return m;
}

} // namespace primitives
} // namespace sg
