#ifndef DS4_HID_KEY_MAP_H
#define DS4_HID_KEY_MAP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct js_key {
    uint8_t size;
    uint8_t *t_id;
    char **t_name;
};

struct js_config {
    char *label;
    struct js_key *axis;
    struct js_key *button;
};

struct joystick {
    int16_t *axis_value;
    uint8_t *button_value;
    struct js_config *config;
};

typedef void(*js_event)(int fd, struct joystick *);

int js_setup_input(const char *node, struct joystick *js, js_event ie);

int js_setup_output(const char *node, struct joystick *js, js_event oe);

int js_bind();

int js_loop();

void js_release();

struct joystick *key_alloc(uint8_t axis, uint8_t buttons, struct js_config *config);

void key_free(struct joystick *js);

void js_print(struct joystick *js);

int key_do_map(struct joystick *in, struct joystick *out, struct key_map *map);

#endif //DS4_HID_KEY_MAP_H
