// TextureSet.h: the renderer's material textures, in one place.
//
// Pulled out of SceneView so the view's draw loop doesn't carry OpenGL texture
// setup. A TextureSet:
//   - loads one PNG per building Material into a GL texture (once), and
//   - binds the right texture for a terminal and sets up automatic texture-
//     coordinate generation, then unbinds it again.
// The building Materials come from grammar/ShapeNode.h (None/Wood/Glass/Stone).

#ifndef VIEW_TEXTURESET_H
#define VIEW_TEXTURESET_H

#include "grammar/ShapeNode.h" // Material
#include "scene/V3.h"          // V3 (terminal size, for tiling)

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
     * Bind `material`'s texture and switch on automatic texture-coordinate
     * generation, scaled by the terminal's `size` so the texture tiles about
     * once per world unit (consistent scale across big walls and tiny trim).
     * Precondition: has(material) is true.
     */
    void bind(Material material, const V3& size) const;

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
    // None=0, Wood=1, Glass=2, Stone=3). 0 means "no texture" -> flat color.
    unsigned int textures[4] = {0, 0, 0, 0};
};

} // namespace sg

#endif // VIEW_TEXTURESET_H
