#pragma once

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

struct Game_Input
{
    f32 dt;
    f32 time;
    Arena *memory;

    Controller controllers[4];
};

struct Game_Output
{
    i32 width;
    i32 height;
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

void GameUpdateAndRender(Game_Input *input, Game_Output *out);
