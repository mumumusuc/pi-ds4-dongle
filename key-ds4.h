#ifndef DS4_HID_KEY_DS4_H
#define DS4_HID_KEY_DS4_H

#include "joystick.h"

#define DS4_AXIS_SIZE   8
#define DS4_BTN_SIZE    13

enum DS4_AXIS {
    DS4_AXIS_L_X = 0,
    DS4_AXIS_L_Y,
    DS4_AXIS_R_X,
    DS4_AXIS_R_Y,
    DS4_AXIS_HAT_X,
    DS4_AXIS_HAT_Y,
    DS4_AXIS_L2,
    DS4_AXIS_R2,
};

enum DS4_BUTTON {
    DS4_BUTTON_X = 0,
    DS4_BUTTON_O,
    DS4_BUTTON_T,
    DS4_BUTTON_B,
    DS4_BUTTON_L1,
    DS4_BUTTON_R1,
    DS4_BUTTON_L2,
    DS4_BUTTON_R2,
    DS4_BUTTON_SHARE,
    DS4_BUTTON_OPTIONS,
    DS4_BUTTON_PS,
    DS4_BUTTON_LS,
    DS4_BUTTON_RS,
};

static const uint8_t ds4_axis_ids[] = {
        DS4_AXIS_L_X,
        DS4_AXIS_L_Y,
        DS4_AXIS_L2,
        DS4_AXIS_R_X,
        DS4_AXIS_R_Y,
        DS4_AXIS_R2,
        DS4_AXIS_HAT_X,
        DS4_AXIS_HAT_Y,
};

static const char *ds4_axis_names[] = {
        "left stick X",
        "left stick Y",
        "right stick X",
        "right stick Y",
        "D-PAD X",
        "D-PAD Y",
        "trigger L2",
        "trigger R2",
};

static const uint8_t ds4_button_ids[] = {
        DS4_BUTTON_X,
        DS4_BUTTON_O,
        DS4_BUTTON_T,
        DS4_BUTTON_B,
        DS4_BUTTON_L1,
        DS4_BUTTON_R1,
        DS4_BUTTON_L2,
        DS4_BUTTON_R2,
        DS4_BUTTON_SHARE,
        DS4_BUTTON_OPTIONS,
        DS4_BUTTON_PS,
        DS4_BUTTON_LS,
        DS4_BUTTON_RS,
};

static const char *ds4_button_names[] = {
        "X", "O", "T", "B", "L1", "R1", "L2", "R2", "share", "option", "PS", "LS", "RS",
};

static const struct js_key ds4_key_axis = {
        .size   = DS4_AXIS_SIZE,
        .t_id   = ds4_axis_ids,
        .t_name = ds4_axis_names,
};

static const struct js_key ds4_key_button = {
        .size   = DS4_BTN_SIZE,
        .t_id   = ds4_button_ids,
        .t_name = ds4_button_names,
};

static const struct js_config ds4_key_config = {
        .label  = "Sony Interactive Entertainment Wireless Controller",
        .axis   = &ds4_key_axis,
        .button = &ds4_key_button,
};

#endif //DS4_HID_KEY_DS4_H
