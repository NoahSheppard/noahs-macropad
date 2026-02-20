// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layer_names {
    _BASE = 0, // binary 001
    _L000,     // binary 000
    _L010,     // binary 010
    _L011,     // binary 011
    _L100,     // binary 100
    _L101,     // binary 101
    _L110,     // binary 110
    _L111      // binary 111
};

enum custom_keycodes {
    BIN0 = SAFE_RANGE,
    BIN1,
    BIN2
};

static uint8_t bin_state = 0b001;

// index = binary state: 000,001,010,011,100,101,110,111
static const uint8_t bin_to_layer[8] = {
    _BASE, _BASE, _L010, _L011, _L100, _L101, _L110, _L111
};

static void apply_binary_layer(void) {
    if ((bin_state & 0x07) == 0) {
        bin_state = 0b001; // disallow "none selected", should not be possible?
    }
    layer_move(bin_to_layer[bin_state & 0x07]);
}

#ifdef OLED_ENABLE
enum {
    OLED_W = 128,
    OLED_H = 32,
    OLED_FB_SIZE = (OLED_W * OLED_H) / 8,
};

static uint8_t oled_fb[OLED_FB_SIZE];

static inline void fb_clear(void) { memset(oled_fb, 0, sizeof(oled_fb)); }

static inline void fb_set_pixel(uint8_t x, uint8_t y, bool on) {
    if (x >= OLED_W || y >= OLED_H) return;
    uint16_t index = x + (y / 8) * OLED_W;
    uint8_t mask = 1 << (y % 8);
    if (on) oled_fb[index] |= mask;
    else    oled_fb[index] &= ~mask;
}

static void fb_hline(uint8_t x, uint8_t y, uint8_t width, bool on) {
    for (uint8_t column = 0; column < width; column++) {
        fb_set_pixel(x + column, y, on);
    }
}

static void fb_vline(uint8_t x, uint8_t y, uint8_t height, bool on) {
    for (uint8_t row = 0; row < height; row++) {
        fb_set_pixel(x, y + row, on);
    }
}

static void fb_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool on) {
    if (width == 0 || height == 0) return;
    fb_hline(x, y, width, on);
    fb_hline(x, y + height - 1, width, on);
    fb_vline(x, y, height, on);
    fb_vline(x + width - 1, y, height, on);
}

static void render_oled_pixels(void) {
    fb_clear();

    fb_rect(0, 0, OLED_W, OLED_H, true);

    for (uint8_t bit = 0; bit < 3; bit++) {
        uint8_t x = 6 + (bit * 8);
        uint8_t y_bottom = OLED_H - 3;
        uint8_t bar_height = (bin_state & (1 << bit)) ? 12 : 4;
        for (uint8_t row = 0; row < bar_height; row++) {
            fb_hline(x, y_bottom - row, 5, true);
        }
    }

    uint8_t scan_x = (timer_read32() / 20) % OLED_W;
    fb_vline(scan_x, 1, OLED_H - 2, true);
}

bool oled_task_user(void) {
    render_oled_pixels();
    oled_write_raw((const char *)oled_fb, sizeof(oled_fb));
    return false; 
}
#endif

#ifdef RGBLIGHT_ENABLE
static void render_bin_indicators(void) {
    for (uint8_t i = 0; i < 3; i++) {
        if (bin_state & (1 << i)) {
            rgblight_setrgb_at(0xFF, 0xFF, 0xFF, i);
        } else {
            rgblight_setrgb_at(0x00, 0x00, 0x00, i);
        }
    }
}

static void render_wave_tail(void) {
    uint8_t base_hue = (timer_read() / 8) & 0xFF;
    for (uint8_t i = 3; i < RGBLIGHT_LED_COUNT; i++) {
        uint8_t hue = base_hue + ((i - 3)  * 10);
        rgblight_sethsv_at(hue, 255, 180, i);
    }
}
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L000] = LAYOUT(
        BIN0, BIN1, BIN2, KC_1,
        KC_2, KC_3, KC_4, KC_5
    ),
    [_L010] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L011] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L100] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L101] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L110] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L111] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
};

void keyboard_post_init_user(void) {
#ifdef RGBLIGHT_ENABLE
    rgblight_enable_noeeprom();
#endif
    apply_binary_layer();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case BIN0:
            bin_state ^= 0b001;
            apply_binary_layer();
            return false;
        case BIN1:
            bin_state ^= 0b010;
            apply_binary_layer();
            return false;
        case BIN2:
            bin_state ^= 0b100;
            apply_binary_layer();
            return false;
    }
    return true;
}

void housekeeping_task_user(void) {
#ifdef RGBLIGHT_ENABLE
    render_wave_tail();
    render_bin_indicators();
#endif
}
