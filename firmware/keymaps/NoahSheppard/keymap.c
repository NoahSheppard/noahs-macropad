// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┐
     * │ N │ O │ A │ H │
     * ├───┼───┼───┼───┤
     * │ B │ E │ N │ J │
     * └───┴───┴───┴───┘
     */
    [0] = LAYOUT(
        KC_N,   KC_O,   KC_A,  KC_H,
        KC_B,   KC_E,   KC_N,  KC_J
    )
};
