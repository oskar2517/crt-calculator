#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <ESP_8_BIT_composite.h>

// ESP_8_BIT_GFX does not expose its composite video object. Include only this
// header in files that need direct access to the back-buffer line pointers.
#pragma push_macro("private")
#define private public
#include <ESP_8_BIT_GFX.h>
#pragma pop_macro("private")

#include "video.h"

inline uint8_t** video_framebuffer_lines() {
    return video_out._pVideo->getFrameBufferLines();
}
