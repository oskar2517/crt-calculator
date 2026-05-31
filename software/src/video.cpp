#include <ESP_8_BIT_GFX.h>

ESP_8_BIT_GFX video_out(false /* = PAL */, 8 /* = RGB332 color */);

uint16_t screen_height = video_out.height();
uint16_t screen_width = video_out.width();