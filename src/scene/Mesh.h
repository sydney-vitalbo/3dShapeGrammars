// Mesh.h
// Sydney Vitalbo

#ifndef SCENE_MESH_H
#define SCENE_MESH_H

#include <cstddef> // size_t
#include <vector>

//https://learnopengl.com/Model-Loading/Mesh

namespace sg {

/**
 * CPU-side triangle mesh (positions + normals)
 */
class Mesh {
public:
    //single vertex: a position and the surface normal at that position
    struct Vertex {
        float px, py, pz; // position
        float nx, ny, nz; // normal (unit length)
    };

    Mesh() = default;

    //Append a vertex and return its index
    unsigned int addVertex(float px, float py, float pz,
                           float nx, float ny, float nz);

    //Add a triangle referencing three previously-added vertices
    void addTriangle(unsigned int a, unsigned int b, unsigned int c);

    //Remove all geometry from mesh
    void clear();

    bool empty() const { return indices.empty(); }
    size_t vertexCount() const { return vertices.size(); }
    size_t triangleCount() const { return indices.size() / 3; }

    /**
     * Draw the mesh using the current OpenGL modelview/projection.
     * The caller is responsible for setting color/material and transforms before this
     */
    void draw() const;

private:
    //   vertices: each stored once
    //   indices: each group of three define one triangle
    
    // (a box corner shared by several faces can live once in vertices and be referenced many times from indices)
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

} // namespace sg

#endif // SCENE_MESH_H
