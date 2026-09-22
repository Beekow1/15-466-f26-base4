#include "TextRenderer.hpp"
#include "ColorTextureProgram.hpp"

#include <hb-ft.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>

constexpr int AtlasSize = 128;
constexpr int CellSize = 16;
constexpr int Columns = AtlasSize / CellSize;

TextRenderer::TextRenderer(std::string const &font_path)
{

    // ref: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    // copying this reference might not be the best way to handle errors? Maybe replace if I build on this later.
    FT_Error ft_error;
    if (ft_error = FT_Init_FreeType(&library))
        abort();
    if (ft_error = FT_New_Face(library, font_path.c_str(), 0, &face))
        abort();
    if (ft_error = FT_Select_Size(face, 0))
        abort();

    if (!face->charmap)

        FT_Set_Charmap(face, face->charmaps[0]);

    font = hb_ft_font_create(face, nullptr);

    size_t glyph_count = static_cast<size_t>(face->num_glyphs);
    std::vector<MeshBuffer::Vertex> vertices;
    std::vector<uint8_t> pixels(AtlasSize * AtlasSize * 4, 0);

    for (size_t id = 0; id < glyph_count; ++id)
    {
        if (ft_error = FT_Load_Glyph(face, static_cast<FT_UInt>(id), FT_LOAD_RENDER))
            abort();

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap const &bitmap = slot->bitmap;

        int width = static_cast<int>(bitmap.width);
        int height = static_cast<int>(bitmap.rows);

        int ox = static_cast<int>(id % Columns) * CellSize + 1;
        int oy = static_cast<int>(id / Columns) * CellSize + 1;

        glm::vec2 uv0 = glm::vec2(float(ox), float(oy)) / float(AtlasSize);
        glm::vec2 uv1 = glm::vec2(float(ox + width), float(oy + height)) / float(AtlasSize);

        float left = float(slot->bitmap_left);
        float top = -float(slot->bitmap_top);
        float right = left + float(width);
        float bottom = top + float(height);

        glm::vec3 const normal(0.0f, 0.0f, 1.0f);
        glm::u8vec4 const white(255, 255, 255, 255);

        MeshBuffer::Vertex a{{left, top, 0.0f}, normal, white, uv0};
        MeshBuffer::Vertex b{{right, top, 0.0f}, normal, white, {uv1.x, uv0.y}};
        MeshBuffer::Vertex c{{right, bottom, 0.0f}, normal, white, uv1};
        MeshBuffer::Vertex d{{left, bottom, 0.0f}, normal, white, {uv0.x, uv1.y}};

        planes.emplace_back(TextRenderer::GlyphPlane{.first = static_cast<GLint>(vertices.size()),
                                                     .count = 6});
        vertices.emplace_back(a);
        vertices.emplace_back(b);
        vertices.emplace_back(c);
        vertices.emplace_back(a);
        vertices.emplace_back(c);
        vertices.emplace_back(d);

        for (int y = 0; y < height; ++y)
        {
            int source_y = bitmap.pitch < 0 ? height - 1 - y : y;
            auto const *row = bitmap.buffer + source_y * (bitmap.pitch < 0 ? -bitmap.pitch : bitmap.pitch);

            for (int x = 0; x < width; ++x)
            {
                size_t dst = (size_t(oy + y) * AtlasSize + ox + x) * 4;

                pixels[dst + 0] = pixels[dst + 1] = pixels[dst + 2] = 255; // todo change if we want colors maybe?
                pixels[dst + 3] = static_cast<uint8_t>((row[x / 8] & 128 >> (x % 8)) != 0) * 255;
            }
        }
    }

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, AtlasSize, AtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshBuffer::Vertex), vertices.data(), GL_STATIC_DRAW);
    auto const &program = *color_texture_program;

    glVertexAttribPointer(program.Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(MeshBuffer::Vertex), reinterpret_cast<void *>(offsetof(MeshBuffer::Vertex, Position)));
    glEnableVertexAttribArray(program.Position_vec4);

    glVertexAttribPointer(program.TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshBuffer::Vertex), reinterpret_cast<void *>(offsetof(MeshBuffer::Vertex, TexCoord)));
    glEnableVertexAttribArray(program.TexCoord_vec2);

    glDisableVertexAttribArray(program.Color_vec4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TextRenderer::draw_text(std::string const &text, glm::vec2 origin, int scale, glm::vec4 color, glm::uvec2 drawable_size) const
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto const &program = *color_texture_program;
    glUseProgram(program.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glVertexAttrib4fv(program.Color_vec4, glm::value_ptr(color));

    float baseline = float(face->size->metrics.ascender) / 64.0f;
    float line_height = float(face->size->metrics.height) / 64.0f + 2.0f;

    glm::mat4 transform(1.0f);
    transform[0][0] = 2.0f * float(scale) / float(drawable_size.x);
    transform[1][1] = -2.0f * float(scale) / float(drawable_size.y);

    std::istringstream lines(text);
    std::string line;

    hb_buffer_t *hb_buffer;

    while (std::getline(lines, line))
    {
        // ref: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
        hb_buffer = hb_buffer_create();

        hb_buffer_add_utf8(hb_buffer, line.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buffer);
        hb_shape(font, hb_buffer, nullptr, 0);

        unsigned int len = hb_buffer_get_length(hb_buffer);
        auto const *info = hb_buffer_get_glyph_infos(hb_buffer, nullptr);
        auto const *pos = hb_buffer_get_glyph_positions(hb_buffer, nullptr);

        glm::vec2 pen(0.0f, baseline);

        for (unsigned i = 0; i < len; ++i)
        {
            GlyphPlane const &plane = planes[info[i].codepoint];

            float x = origin.x + float(scale) * float(pen.x + float(pos[i].x_offset) / 64.);
            float y = origin.y + float(scale) * float(pen.y - float(pos[i].y_offset) / 64.);

            transform[3][0] = 2.0f * x / float(drawable_size.x) - 1.0f;
            transform[3][1] = 1.0f - 2.0f * y / float(drawable_size.y);

            if (plane.count != 0)
            {
                glUniformMatrix4fv(program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(transform));
                glDrawArrays(GL_TRIANGLES, plane.first, plane.count);
            }

            pen.x += float(pos[i].x_advance / 64.);
            pen.y -= float(pos[i].y_advance / 64.);
        }

        baseline += line_height;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

TextRenderer::~TextRenderer()
{
}