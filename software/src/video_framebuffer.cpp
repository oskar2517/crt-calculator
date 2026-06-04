#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <ESP_8_BIT_composite.h>

// ESP_8_BIT_GFX does not expose its composite video object. Keep this private
// access workaround in one translation unit so callers are not include-order
// dependent.
#pragma push_macro("private")
#define private public
#include <ESP_8_BIT_GFX.h>
#pragma pop_macro("private")

#include "video.h"
#include "video_framebuffer.h"

uint8_t** video_framebuffer_lines() {
    return video_out._pVideo->getFrameBufferLines();
}
