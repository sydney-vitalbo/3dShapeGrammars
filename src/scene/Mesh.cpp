//Mesh.cpp

#include "scene/Mesh.h"

#include <cmath>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

namespace sg {

unsigned int Mesh::addVertex(float px, float py, float pz,
                             float nx, float ny, float nz) {
    vertices.push_back(Vertex{px, py, pz, nx, ny, nz});
    return static_cast<unsigned int>(vertices.size() - 1);
}

void Mesh::addTriangle(unsigned int a, unsigned int b, unsigned int c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

void Mesh::clear() {
    vertices.clear();
    indices.clear();
}

void Mesh::draw(float width, float height, float depth) const {
    glBegin(GL_TRIANGLES);
    for (unsigned int idx : indices) {
        const Vertex& v = vertices[idx];
        // Project onto the face's two tangent axes, using scaled dimensions.
        // A single X/Y projection collapses on side and horizontal faces.
        if (std::abs(v.nx) > 0.5f)
            glTexCoord2f(v.pz * depth, v.py * height);
        else if (std::abs(v.ny) > 0.5f)
            glTexCoord2f(v.px * width, v.pz * depth);
        else
            glTexCoord2f(v.px * width, v.py * height);
        glNormal3f(v.nx, v.ny, v.nz); // normal for the vertex emitted next
        glVertex3f(v.px, v.py, v.pz); // emit the corner point
    }
    glEnd();
}

} // namespace sg
