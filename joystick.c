#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/joystick.h>
#include "joystick.h"


struct joystick *key_alloc(uint8_t axis, uint8_t buttons, struct js_config *config) {
    struct joystick *js = (struct joystick *) calloc(1, sizeof(struct joystick));
    if (!js) {
        perror("Alloc failed");
        goto failed;
    }
    js->axis_value = (int16_t *) calloc(1, axis);
    if (!js->axis_value) {
        perror("Alloc failed");
        goto free_js;
    }
    js->button_value = (uint8_t *) calloc(1, buttons);
    if (!js->button_value) {
        perror("Alloc failed");
        goto free_axis;
    }
    js->config = config;
    return js;

    free_axis:
    free(js->axis_value);
    free_js:
    free(js);
    failed:
    return NULL;
}

__always_inline void key_free(struct joystick *js) {
    free(js->axis_value);
    free(js->button_value);
    free(js);
}

__always_inline void js_print(struct joystick *js) {
    struct js_config *config = js->config;
    puts("Axis:");
    for (int i = 0; i < config->axis->size; i++) {
        printf("\t[%d]\t%s\t%d\n", i, config->axis->t_name[i], js->axis_value[i]);
    }
    puts("\nButtons:");
    for (int i = 0; i < config->button->size; i++) {
        printf("\t[%d]\t%s\t%u\n", i, config->button->t_name[i], js->button_value[i]);
    }
    puts("");
}


struct js_dev {
    int fd;
    js_event event;
    struct joystick *dev;
};

static struct js_dev *js_input;
static struct js_dev *js_output;

static int js_check_params(int fd, struct joystick *js) {
#define JS_NAME_SIZE 64
    int ret;
    uint8_t num_axis, num_buttons;
    uint8_t js_name[JS_NAME_SIZE];
    /* check name */
    ret = ioctl(fd, JSIOCGNAME(JS_NAME_SIZE), js_name);
    if (ret < 0) {
        perror("JSIOCGNAME");
        return ret;
    }
    printf("Name : %s\n", js_name);
    /* check axes */
    ret = ioctl(fd, JSIOCGAXES, &num_axis);
    if (ret < 0) {
        perror("JSIOCGAXES");
        return ret;
    }
    printf("Axis : %u\n", num_axis);
    /* check buttons */
    ret = ioctl(fd, JSIOCGBUTTONS, &num_buttons);
    if (ret < 0) {
        perror("JSIOCGBUTTONS");
        return ret;
    }
    printf("Buttons : %u\n", num_buttons);

    if (num_axis != js->config->axis->size || num_buttons != js->config->button->size)
        return -ENODEV;
}

int js_setup_input(const char *node, struct joystick *js, js_event ie) {
    int ret;
    if (!js)
        return -EINVAL;
    int fd = open(node, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Open %s failed : %s\n", node, strerror(errno));
        return fd;
    }
    js_input = (struct js_dev *) calloc(1, sizeof(*js_input));
    if (!js_input) {
        fprintf(stderr, "Alloc input device failed : %s\n", strerror(errno));
        ret = -ENOMEM;
        goto done;
    }
    js_input->fd = fd;
    js_input->dev = js;
    js_input->event = ie;
    ret = js_check_params(fd, js);
    if (ret < 0)
        goto free;
    return 0;

    free:
    free(js_input);
    done:
    close(fd);
    return ret;
}

int js_setup_output(const char *node, struct joystick *js, js_event oe) {
    int ret;
    if (!js)
        return -EINVAL;
    int fd = open(node, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Open %s failed : %s\n", node, strerror(errno));
        return fd;
    }
    js_output = (struct js_dev *) calloc(1, sizeof(*js_output));
    if (!js_output) {
        fprintf(stderr, "Alloc output device failed : %s\n", strerror(errno));
        ret = -ENOMEM;
        goto done;
    }
    js_output->fd = fd;
    js_output->dev = js;
    js_output->event = oe;
    return 0;

    done:
    close(fd);
    return ret;
}

__always_inline static int do_axis_map(uint8_t raw_num, int16_t raw_value) {
// raw_num -> ds4_index -> ns_index
    uint8_t i_in = js_input->dev->config->axis->t_id[raw_num];
    if (i_in >= js_output->dev->config->axis->size)
        return -ENODATA;
    uint8_t i_out = js_output->dev->config->axis->t_id[i_in];
    js_output->dev->axis_value[i_out] = raw_value;
    return 0;
}

__always_inline static int do_button_map(uint8_t raw_num, int16_t raw_value) {
// raw_num -> ds4_index -> ns_index
    uint8_t i_in = js_input->dev->config->button->t_id[raw_num];
    if (i_in >= js_output->dev->config->button->size)
        return -ENODATA;
    uint8_t i_out = js_output->dev->config->button->t_id[i_in];
    js_output->dev->button_value[i_out] = raw_value;
    return 0;
}

int js_loop() {
    int ret;
    struct js_event event;
    if (!js_input || !js_output)
        return -EINVAL;

    while (1) {
        ret = read(js_input->fd, &event, sizeof(event));
        if (ret < 0)
            return ret;
        switch (event.type) {
            case JS_EVENT_AXIS:
                ret = do_axis_map(event.number, event.value);
                if (ret < 0)
                    continue;
                break;
            case JS_EVENT_BUTTON:
                ret = do_button_map(event.number, event.value);
                if (ret < 0)
                    continue;
                break;
            default:
                continue;
        }
        if (js_input->event)
            js_input->event(js_input->fd, js_input->dev);
        if (js_output->event)
            js_output->event(js_output->fd, js_output->dev);
    }
}


__always_inline void js_release() {
    close(js_input->fd);
    free(js_input);
    close(js_output->fd);
    free(js_output);
}