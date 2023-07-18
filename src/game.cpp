#pragma once

#include "game.h"

void DrawRect(Game_Output *out, Rectangle2 rect, Vector4 color) {
    u32 * pixels = (u32 *) out->pixels;

    rect = abs_r2(rect);

    i32 in_x0 = Clamp((i32) rect.x0, 0, out->width);
    i32 in_x1 = Clamp((i32) rect.x1, 0, out->width);

    i32 in_y0 = Clamp((i32) rect.y0, 0, out->height);
    i32 in_y1 = Clamp((i32) rect.y1, 0, out->height);

    u32 out_color = u32_from_v4(color);


    u32 * at = &pixels[in_y0 * out->width + in_x0];

    for (i32 y = in_y0; y < in_y1; y += 1) {
        for (i32 x = in_x0; x < in_x1; x += 1) {
            //pixels[y * out->width + x] = out_color;
            *at = out_color;
            at += 1;
        }

        at += out->width - (in_x1 - in_x0);
    }
}

void DrawCircle(Game_Output *out, Vector2 center, f32 radius, Vector4 color) {
    u32 * pixels = (u32 *) out->pixels;
    u32 out_color = u32_from_v4(color);

    i32 in_x0 = Clamp((i32) (center.x - radius), 0, out->width);
    i32 in_x1 = Clamp((i32) (center.x + radius), 0, out->width);
    i32 in_y0 = Clamp((i32) (center.y - radius), 0, out->height);
    i32 in_y1 = Clamp((i32) (center.y + radius), 0, out->height);

    for (i32 y = in_y0; y < in_y1; y += 1) {
        for (i32 x = in_x0; x < in_x1; x += 1) {
            Vector2 pixel_pos = {(f32) x, (f32) y};
            f32 distance = v2_length(center - pixel_pos);

            if (distance <= radius) {
                u32 * at = &pixels[y * out->width + x];
                *at = out_color;
            }
        }
    }
}

void DrawLine(Game_Output *out, Vector2 start, Vector2 end, Vector4 color) {
    u32 * pixels = (u32 *) out->pixels;
    u32 out_color = u32_from_v4(color);

    f32 delta_x = end.x - start.x;
    f32 delta_y = end.y - start.y;
    f32 length = Max(Abs(delta_x), Abs(delta_y));

    f32 step_x = delta_x / length;
    f32 step_y = delta_y / length;

    f32 x = start.x;
    f32 y = start.y;

    for (i32 i = 0; i < length; i++) {
        i32 in_x = Clamp((i32) x, 0, out->width);
        i32 in_y = Clamp((i32) y, 0, out->height);

        u32 * at = &pixels[in_y * out->width + in_x];
        *at = out_color;

        x += step_x;
        y += step_y;
    }
}


bond::obj_result draw_rect(const bond::t_vector &args) {
    Rectangle2Wrapper *rect;
    Vector4Wrapper *color;
    GameWindow *out;

    TRY(bond::parse_args(args, out, rect, color));
    DrawRect(out->output, rect->m_vector, color->m_vector);
    return bond::OK();
}

bond::obj_result draw_circle(const bond::t_vector &args) {
    Vector2Wrapper *center;
    bond::Float *radius;
    Vector4Wrapper *color;
    GameWindow *out;

    TRY(bond::parse_args(args, out, center, radius, color));
    DrawCircle(out->output, center->m_vector, (f32) radius->get_value(), color->m_vector);
    return bond::OK();
}


bool load_font(Font *font, const char *filename, f32 fontSize) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    font->font_buffer.resize(file_size);
    fread(font->font_buffer.data(), 1, file_size, file);
    fclose(file);

    if (!stbtt_InitFont(&font->font_info, font->font_buffer.data(), 0)) {
        return false;
    }

    font->scale = stbtt_ScaleForPixelHeight(&font->font_info, fontSize);
    return true;
}

void render_text(Game_Output *out, const char *text, Vector2 pos, Vector4 color) {
    u32 * pixels = (u32 *) out->pixels;
    u32 out_color = u32_from_v4(color);

    Font *font = &out->default_font;

    f32 x = pos.x;
    f32 y = pos.y;

    while (*text) {
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font->font_info, *text, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font->font_info, *text, font->scale, font->scale, &x0, &y0, &x1, &y1);

        int w = x1 - x0;
        int h = y1 - y0;

        int byte_offset = (int) (x + x0 + (y + y0) * out->width);
        u8 *bitmap = stbtt_GetCodepointBitmap(&font->font_info, font->scale, font->scale, *text, &w, &h, 0, 0);

        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i += 1) {
                u8 alpha = bitmap[j * w + i];
                if (alpha) {
                    u32 * at = &pixels[byte_offset + i];
                    *at = out_color;
                }
            }

            byte_offset += out->width;
        }

        x += advance * font->scale;
        text += 1;
    }
}

static auto ball_position = v2(0, 0);
static auto ball_velocity = v2(10, 10);

void GameUpdateAndRender(Game_Input *input, Game_Output *out) {

    static auto ball_radius = 10.0f;

    if (ball_position.x < 0 || ball_position.x > out->width) {
        ball_velocity.x *= -1;
    }

    if (ball_position.y < 0 || ball_position.y > out->height) {
        ball_velocity.y *= -1;
    }

    ball_position = v2_add(ball_position, ball_velocity);
    print("ball_position: %f, %f\n", ball_position.x, ball_position.y);

    DrawCircle(out, ball_position, ball_radius, v4(1, 1, 1, 1));

    print("down: %d\n", input->controllers[0].down);
}

bond::obj_result Vector4Wrapper::constructor(const bond::t_vector &args) {
    bond::Float *x;
    bond::Float *y;
    bond::Float *z;
    bond::Float *w;

    TRY(bond::parse_args(args, x, y, z, w));
    return bond::make<Vector4Wrapper>(
            Vector4{(f32) x->get_value(), (f32) y->get_value(), (f32) z->get_value(), (f32) w->get_value()});
}

bond::obj_result Vector2Wrapper::constructor(const bond::t_vector &args) {
    bond::Float *x, *y;
    TRY(bond::parse_args(args, x, y));
    return bond::make<Vector2Wrapper>(Vector2{(f32) x->get_value(), (f32) y->get_value()});
}

bond::obj_result Rectangle2Wrapper::constructor(const bond::t_vector &args) {
    bond::Float *x0;
    bond::Float *y0;
    bond::Float *x1;
    bond::Float *y1;


    TRY(bond::parse_args(args, x0, y0, x1, y1));
    return bond::make<Rectangle2Wrapper>(
            Rectangle2{(f32) x0->get_value(), (f32) y0->get_value(), (f32) x1->get_value(), (f32) y1->get_value()});
}
