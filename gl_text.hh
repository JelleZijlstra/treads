#pragma once

#include <stdio.h>

#include <phosg/Image.hh>

void draw_text(float x, float y, float r, float g, float b, float a,
    float aspect_ratio, float char_size, bool centered, const char* fmt, ...);

void render_image(const Image& img, float x1, float x2, float y1, float y2,
    float alpha, bool gl_begin);
