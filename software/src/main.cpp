#include <Arduino.h>
#include <stdint.h>
#include <LittleFS.h>

#include "bad_apple.h"
#include "calculator.h"
#include "keypad.h"
#include "snake.h"
#include "video.h"

#define MODE_SWITCH_WINDOW_MS UINT32_C(350)

typedef struct {
    void (*enter)();
    void (*exit)();
    void (*render)();
    void (*handle_key)(char read_key);
} AppMode;

static uint8_t current_mode_index = 0;
static uint32_t last_clear_press = 0;

static AppMode app_modes[] = {
    {calculator_enter, calculator_exit, calculator_render, calculator_handle_key},
    {snake_enter, snake_exit, snake_render, snake_handle_key},
    {bad_apple_enter, bad_apple_exit, bad_apple_render, bad_apple_handle_key},
};

static uint8_t app_mode_count() {
    return sizeof(app_modes) / sizeof(app_modes[0]);
}

static AppMode* current_mode() { return &app_modes[current_mode_index]; }

static void switch_mode(uint8_t mode_index) {
    current_mode()->exit();
    current_mode_index = mode_index % app_mode_count();
    current_mode()->enter();
}

static void switch_to_next_mode() { switch_mode(current_mode_index + 1); }

static bool handle_mode_shortcut(char read_key) {
    if (read_key != 'C') {
        last_clear_press = 0;
        return false;
    }

    uint32_t now = millis();

    if (last_clear_press != 0 &&
        now - last_clear_press <= MODE_SWITCH_WINDOW_MS) {
        last_clear_press = 0;
        switch_to_next_mode();
        return true;
    }

    last_clear_press = now;
    return false;
}

void setup() {
    Serial.begin(9600);
    video_out.begin();
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    current_mode()->enter();

    // Enable 12V boost converter
    pinMode(26, OUTPUT);
    digitalWrite(26, HIGH);
}

void loop() {
    AppMode* mode = current_mode();

    mode->render();

    char read_key = keypad.getKey();

    if (!read_key) return;

    if (handle_mode_shortcut(read_key)) return;

    mode->handle_key(read_key);
}
