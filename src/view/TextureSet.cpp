// TextureSet.cpp
// Material textures for the renderer: PNG loading + per-terminal binding.
// Extracted from SceneView so the draw loop stays focused on shapes

#include "view/TextureSet.h"

#include <FL/Fl_PNG_Image.H> // FLTK's bundled PNG decoder (no extra dependency)

#include <cstddef> // size_t
#include <string>  // build candidate texture paths
#include <vector>  // repack pixel rows for glTexImage2D

//macOS compatibility (same GL headers SceneView uses)
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

namespace sg {

// Decode one PNG with FLTK and upload it as an OpenGL texture for `material`.
// `relativePath` is given relative to the repository root (e.g.
// "src/textures/Stone.png"); loadAll() tries it under several base directories
// so the texture is found no matter which folder the program is launched from.
//https://www.fltk.org/newsgroups.php?s0+gfltk.general+v39519+T0
void TextureSet::loadTexture(Material material, const char* relativePath) {
    // Relative paths resolve against the CURRENT WORKING DIRECTORY, which is not
    // the same in every way the app gets launched so build a list of candidate
    // paths and use the first that decodes
    std::string fromRoot = relativePath;
    std::string leaf = fromRoot;
    if (leaf.rfind("src/", 0) == 0) leaf = leaf.substr(4); // strip leading "src/"

    const std::string candidates[] = {
        fromRoot,              // cwd = repo root (CMake run/debug)
        leaf,                  // cwd = src/  (running src/main.exe from src/)
        "../" + fromRoot,      // cwd one level below repo root
        "../../" + fromRoot,   // cwd = build/<preset>/bin
        "../../../" + fromRoot // cwd = build/<preset>/bin/<config>
    };

    for (const std::string& full : candidates) {
        Fl_PNG_Image img(full.c_str());
        if (img.fail() || img.w() <= 0 || img.h() <= 0) continue; // try next candidate
        uploadTexture(material, img);
        return; // loaded successfully
    }
    // None of the candidate paths worked -> leave this material untextured
    // (the renderer falls back to a flat color for it).
}

// Upload an already-decoded FLTK image as the GL texture for `material`.
void TextureSet::uploadTexture(Material material, Fl_RGB_Image& img) {

    const int w = img.w();
    const int h = img.h();
    const int d = img.d();  // channels: 1=gray,3=RGB,4=RGBA
    const int ld = img.ld() ? img.ld() : w * d; // row stride (may include padding)
    const unsigned char* src = reinterpret_cast<const unsigned char*>(img.data()[0]);

    // Repack into RGB/RGBA
    const int outCh = (d == 4) ? 4 : 3;
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * outCh);
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = src + static_cast<size_t>(y) * ld;
        unsigned char* out = pixels.data() + static_cast<size_t>(y) * w * outCh;
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = row + static_cast<size_t>(x) * d;
            if (d == 1) { out[0] = out[1] = out[2] = p[0]; }       // gray -> RGB
            else { out[0]=p[0]; out[1]=p[1]; out[2]=p[2]; if (outCh==4) out[3]=p[3]; }
            out += outCh;
        }
    }

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // our buffer is tightly packed
    const GLint fmt = (outCh == 4) ? GL_RGBA : GL_RGB;
    // gluBuild2DMipmaps handles non-power-of-two sizes and builds mipmaps, so the
    // texture stays clean when the building is small/far on screen.
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    textures[static_cast<int>(material)] = tex;
}

// Load one texture per building material
void TextureSet::loadAll() {
    loadTexture(Material::Stone, "src/textures/Stone.png");
    loadTexture(Material::Glass, "src/textures/Glass.png");
    loadTexture(Material::Wood,  "src/textures/Wood.png");
    // Material::None (roof deck) stays untextured -> flat color.
}

bool TextureSet::has(Material material) const {
    return textures[static_cast<int>(material)] != 0;
}

void TextureSet::bind(Material material, const V3& size) const {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[static_cast<int>(material)]);

    //s follows object X, t follows object Y. Folding the terminal's size
    //into the planes makes the texture tile ~once per world unit, so the
    //scale looks consistent across big walls and tiny trim pieces
    V3 s = size; // V3's subscript operator is non-const, so copy to index
    const GLfloat sPlane[4] = {s[0], 0.0f, 0.0f, 0.0f};
    const GLfloat tPlane[4] = {0.0f, s[1], 0.0f, 0.0f};
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
}

void TextureSet::unbind() const {
    //turn texturing back off so the next piece (or the grid/highlight) isn't accidentally textured
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
}

} // namespace sg
