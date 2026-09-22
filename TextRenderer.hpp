#pragma once

#include "GL.hpp"
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include "Mesh.hpp"
#include <string>
#include <vector>

struct TextRenderer
{
    TextRenderer(std::string const &font_path);
    ~TextRenderer();

    void draw_text(std::string const &text, glm::vec2 origin, int scale,
                   glm::vec4 color, glm::uvec2 drawable_size) const;

private:

    struct GlyphPlane
    {
        GLint first = 0;
        GLsizei count = 0; 
    };

    FT_Library library = nullptr;
    FT_Face face = nullptr;
    hb_font_t *font = nullptr;

    GLuint texture = 0, vao = 0, vbo = 0;
    std::vector<GlyphPlane> planes;
};