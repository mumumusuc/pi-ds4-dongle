#ifndef SWITCH_HID_KEY_SWITCH_H
#define SWITCH_HID_KEY_SWITCH_H

#include <memory.h>
#include "joystick.h"

#define NS_AXIS_SIZE    6
#define NS_BTN_SIZE     14

enum SWITCH_AXIS {
    SWITCH_AXIS_L_X = 0,
    SWITCH_AXIS_L_Y,
    SWITCH_AXIS_R_X,
    SWITCH_AXIS_R_Y,
    SWITCH_AXIS_HAT_X,
    SWITCH_AXIS_HAT_Y,
};

/* ZR|ZL|R|L|X|A|B|Y , CAPTURE|HOME|RS|LS|START|SELECT */
enum SWITCH_BUTTON {
    SWITCH_BUTTON_Y = 0,
    SWITCH_BUTTON_B,
    SWITCH_BUTTON_A,
    SWITCH_BUTTON_X,
    SWITCH_BUTTON_L,
    SWITCH_BUTTON_R,
    SWITCH_BUTTON_ZL,
    SWITCH_BUTTON_ZR,
    SWITCH_BUTTON_MINUS,
    SWITCH_BUTTON_PLUS,
    SWITCH_BUTTON_LS,
    SWITCH_BUTTON_RS,
    SWITCH_BUTTON_HOME,
    SWITCH_BUTTON_CAPTURE,
};

static const uint8_t ns_axis_ids[] = {
        SWITCH_AXIS_L_X,
        SWITCH_AXIS_L_Y,
        SWITCH_AXIS_R_X,
        SWITCH_AXIS_R_Y,
        SWITCH_AXIS_HAT_X,
        SWITCH_AXIS_HAT_Y,
};

static const char *ns_axis_names[] = {
        "left stick X",
        "left stick Y",
        "right stick X",
        "right stick Y",
        "D-PAD X",
        "D-PAD Y",
};

static const uint8_t ns_button_ids[] = {
        SWITCH_BUTTON_B,
        SWITCH_BUTTON_A,
        SWITCH_BUTTON_X,
        SWITCH_BUTTON_Y,
        SWITCH_BUTTON_L,
        SWITCH_BUTTON_R,
        SWITCH_BUTTON_ZL,
        SWITCH_BUTTON_ZR,
        SWITCH_BUTTON_MINUS,
        SWITCH_BUTTON_PLUS,
        SWITCH_BUTTON_HOME,
        SWITCH_BUTTON_LS,
        SWITCH_BUTTON_RS,
        SWITCH_BUTTON_CAPTURE,
};

static const char *ns_button_names[] = {
        "Y", "B", "A", "X", "L", "R", "ZL", "ZR", "-", "+", "LS", "RS", "home", "capture",
};

static const struct js_key ns_key_axis = {
        .size   = NS_AXIS_SIZE,
        .t_id   = ns_axis_ids,
        .t_name = ns_axis_names,
};

static const struct js_key ns_key_button = {
        .size   = NS_BTN_SIZE,
        .t_id   = ns_button_ids,
        .t_name = ns_button_names,
};

static const struct js_config ns_key_config = {
        .label  = "Nintendo Switch Controller",
        .axis   = &ns_key_axis,
        .button = &ns_key_button,
};


/*
    uint16_t Button; // 16 buttons; see JoystickButtons_t for bit mapping
    uint8_t HAT;    // HAT switch; one nibble w/ unused nibble
    uint8_t LX;     // Left  Stick X
    uint8_t LY;     // Left  Stick Y
    uint8_t RX;     // Right Stick X
    uint8_t RY;     // Right Stick Y
    top     - 0
    top-r   - 1
    right   - 2
    ...
    top-l   - 7
    default - 8
 */

static uint8_t switch_hat(int16_t hat_x, int16_t hat_y) {
    hat_x = hat_x > 1000 ? 1 : (hat_x < -1000 ? -1 : 0);
    hat_y = hat_y > 1000 ? -1 : (hat_y < -1000 ? 1 : 0);
    if (hat_y > 0 && hat_x == 0)
        return 0x00;
    else if (hat_y > 0 && hat_x > 0)
        return 0x01;
    else if (hat_y == 0 && hat_x > 0)
        return 0x02;
    else if (hat_y < 0 && hat_x > 0)
        return 0x03;
    else if (hat_y < 0 && hat_x == 0)
        return 0x04;
    else if (hat_y < 0 && hat_x < 0)
        return 0x05;
    else if (hat_y == 0 && hat_x < 0)
        return 0x06;
    else if (hat_y > 0 && hat_x < 0)
        return 0x07;
    return 0x08;
}

static int switch_input(struct joystick *js, uint8_t *buf) {
    memset(buf, 0, 8);
    uint16_t button = 0;
    for (int i = 0; i < js->config->button->size; i++)
        button |= js->button_value[i] << i;
    buf[0] = button & 0xFF;
    buf[1] = (button >> 8) & 0xFF;
    buf[2] = switch_hat(js->axis_value[SWITCH_AXIS_HAT_X], js->axis_value[SWITCH_AXIS_HAT_Y]);
    buf[3] = (js->axis_value[SWITCH_AXIS_L_X] + (0xffff >> 1)) >> 8;
    buf[4] = (js->axis_value[SWITCH_AXIS_L_Y] + (0xffff >> 1)) >> 8;
    buf[5] = (js->axis_value[SWITCH_AXIS_R_X] + (0xffff >> 1)) >> 8;
    buf[6] = (js->axis_value[SWITCH_AXIS_R_Y] + (0xffff >> 1)) >> 8;
    //buf[7] = 0;
    return 0;
}

#endif //SWITCH_HID_KEY_SWITCH_H
