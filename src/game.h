#pragma once
#include <object.h>


#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

#define COLOR_RED 0xffff0000

struct Controller
{
    b32 up;
    b32 down;
    b32 left;
    b32 right;

    b32 a;
    b32 b;
    b32 start;
    b32 pause;
};


struct Font {
    stbtt_fontinfo font_info;
    std::vector<u8> font_buffer;
    f32 scale;
};

struct Game_Input
{
    f32 dt;
    f32 time;

    Controller controllers[4];
};

struct Game_Output
{
    i32 width;
    i32 height;
    Font default_font;
    u32 *pixels;

    // TODO(nick): sound
};

//
// NOTE(nick): closer to what a "real" graphics backend is doing
// But then you can't express the "GetPixel" function as easily
//

#if 0
struct Vertex
{
    Vector2 pos;
    u32 color;

    //Texture tex;
    //Vector2 uv;
};

struct Triangle
{
    Vertex vertices[3];
};

struct Triangle_List
{
    Triangle *data;
    u32 count;
    u32 capacity;
};

struct Game_Output
{
    Triangle_List triangles;

    // TODO(nick): sound
};
#endif

void DrawRect(Game_Output *out, Rectangle2 r, u32 color);
void DrawCircle(Game_Output *out, Rectangle2 r, u32 color);
void DrawLine(Game_Output *out, Vector2 p0, Vector2 p1, u32 color);

void DrawSetPixel(Game_Output *out, Vector2 pos, u32 color);
u32 DrawGetPixel(Game_Output *out, Vector2 pos);

//
// API
//

void DrawRect(Game_Output *out, Rectangle2 rect, Vector4 color);
void DrawCircle(Game_Output *out, Vector2 center, f32 radius, Vector4 color);
void DrawLine(Game_Output *out, Vector2 start, Vector2 end, Vector4 color);
void GameUpdateAndRender(Game_Input *input, Game_Output *out);
bool load_font(Font *font, const char *filename, f32 fontSize);
void render_text(Game_Output *out, const char *text, Vector2 pos, Vector4 color);


class GameWindow: public bond::NativeInstance {
public:
    GameWindow(Game_Input* input, Game_Output* output) : input(input), output(output) {}
    Game_Input* input;
    Game_Output* output;

    static bond::obj_result get_width(const bond::GcPtr<bond::Object>& self) {
        auto Self = self->as<GameWindow>();
        return bond::make_int(Self->output->width);
    }

    static bond::obj_result get_height(const bond::GcPtr<bond::Object>& self) {
        auto Self = self->as<GameWindow>();
        return bond::make_int(Self->output->height);
    }

};


#define vec_getter(TYPE, VALUE) \
    static bond::obj_result getter_##VALUE(const bond::GcPtr<bond::Object>& self) { \
        auto Self = self->as<TYPE>();                  \
        return bond::make_float(Self->m_vector.VALUE);                            \
    }

#define vec_setter(TYPE, VALUE) \
    static bond::obj_result setter_##VALUE(const bond::GcPtr<bond::Object>& self, const bond::GcPtr<bond::Object>& other) { \
        auto Self = self->as<TYPE>();                                                                             \
        if (!other->is<bond::Float>()) {                                                                                      \
            return bond::ERR("Expected float");                                                                                      \
        }                                                                                                                    \
        Self->m_vector.VALUE = (f32)other->as<bond::Float>()->get_value();                            \
        return self; \
    }

#define getter_setter(TYPE, VALUE) \
    vec_getter(TYPE, VALUE) \
    vec_setter(TYPE, VALUE)

class Vector4Wrapper: public bond::NativeInstance {
public:
    explicit Vector4Wrapper(Vector4 vec) { m_vector = vec; }
    Vector4 m_vector = v4_zero;

    static bond::obj_result constructor(const bond::t_vector& args);
    getter_setter(Vector4Wrapper,x);
    getter_setter(Vector4Wrapper, y);
    getter_setter(Vector4Wrapper, z);
    getter_setter(Vector4Wrapper, w);
};

class Vector2Wrapper: public bond::NativeInstance {
public:
    explicit Vector2Wrapper(Vector2 vec) { m_vector = vec; }
    Vector2 m_vector = v2_zero;

    static bond::obj_result constructor(const bond::t_vector& args);
    getter_setter(Vector2Wrapper, x);
    getter_setter(Vector2Wrapper, y);

    std::string str() const override {
        return "Vector2(" + std::to_string(m_vector.x) + ", " + std::to_string(m_vector.y) + ")";
    }
};

class Rectangle2Wrapper: public bond::NativeInstance {
public:
    explicit Rectangle2Wrapper(Rectangle2 rect) { m_vector = rect; }
    Rectangle2 m_vector;

    static bond::obj_result constructor(const bond::t_vector& args);
    getter_setter(Rectangle2Wrapper, x0);
    getter_setter(Rectangle2Wrapper, y0);
    getter_setter(Rectangle2Wrapper, x1);
    getter_setter(Rectangle2Wrapper, y1);
};

bond::obj_result draw_rect(const bond::t_vector& args);
bond::obj_result draw_circle(const bond::t_vector &args);


