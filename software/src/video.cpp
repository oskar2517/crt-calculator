#include "video.h"

ESP_8_BIT_GFX video_out(false /* = PAL */, 8 /* = RGB332 color */);

uint16_t screen_height = video_out.height();
uint16_t screen_width = video_out.width();

void draw_centered_text(const char* text, int16_t center_x, int16_t y) {
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;

    video_out.getTextBounds(text, 0, y, &x1, &y1, &width, &height);

    // NOTE: -8 corrects for visual offset
    video_out.setCursor(center_x - 8 - width / 2, y);
    video_out.print(text);
}
