// TextureSet.h: the renderer's material textures, in one place.
//
// Pulled out of SceneView so the view's draw loop doesn't carry OpenGL texture
// setup. A TextureSet:
//   - loads one PNG per building Material into a GL texture (once), and
//   - binds the right texture for a terminal, then unbinds it again.
// Mesh supplies face-specific texture coordinates.
// The building Materials come from grammar/ShapeNode.h.

#ifndef VIEW_TEXTURESET_H
#define VIEW_TEXTURESET_H

#include "grammar/ShapeNode.h" // Material

class Fl_RGB_Image; // FLTK decoded image (Fl_PNG_Image derives from it)

namespace sg {

/**
 * Holds one OpenGL texture per Material and applies them while drawing.
 *
 * Lifetime/threading: all methods touch OpenGL, so they must be called with a
 * current GL context (i.e. from inside SceneView's draw()). loadAll() must run
 * once before the first bind(); SceneView does this lazily on its first frame.
 */
class TextureSet {
public:
    TextureSet() = default;

    /**
     * Load every building material's texture from src/textures/*.png.
     * Call once, after the GL context exists. Missing/unreadable files are
     * skipped, leaving that material untextured (the renderer falls back to a
     * flat color for it).
     */
    void loadAll();

    /**
     * True if `material` has a texture loaded (so the caller knows whether to
     * bind a texture or use its flat-color fallback).
     */
    bool has(Material material) const;

    /**
     * Bind `material`'s texture. Mesh::draw supplies coordinates scaled by the
     * terminal dimensions so the texture tiles about once per world unit.
     * Precondition: has(material) is true.
     */
    void bind(Material material) const;

    /**
     * Undo bind(): turn texgen + 2D texturing back off so later draws (the grid,
     * the selection outline, untextured pieces) are not accidentally textured.
     */
    void unbind() const;

private:
    /**
     * Decode one PNG with FLTK and upload it as the GL texture for `material`.
     * `relativePath` is relative to the repository root (e.g.
     * "src/textures/Stone.png"); the file is searched under several base
     * directories so it loads regardless of the current working directory.
     * If no candidate decodes, the material is left untextured.
     */
    void loadTexture(Material material, const char* relativePath);

    /**
     * Upload an already-decoded FLTK image as the GL texture for `material`.
     * Separated from loadTexture so the path-search logic stays readable.
     */
    void uploadTexture(Material material, Fl_RGB_Image& img);

    // One GL texture id per Material (enum values index this array:
    // through RedMarble). 0 means "no texture" -> flat color.
    unsigned int textures[static_cast<int>(Material::RedMarble) + 1] = {};
};

} // namespace sg

#endif // VIEW_TEXTURESET_H
