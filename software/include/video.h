#pragma once

#include <ESP_8_BIT_GFX.h>

extern ESP_8_BIT_GFX video_out;

extern uint16_t screen_height;
extern uint16_t screen_width;

void draw_centered_text(const char* text, int16_t center_x, int16_t y);
