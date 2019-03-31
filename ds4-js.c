#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "joystick.h"
#include "key-ds4.h"
#include "key-switch.h"

static char *js_node = "/dev/input/js0";
static char *sw_fd = "/dev/hidg0";
static uint8_t buf[8];

static int on_event(int fd, struct joystick *js) {
    //js_print(js);
    switch_input(js, buf);
    for (int i = 0; i < 8; i++)
        printf("%02x ", buf[i]);
    puts("");
    write(fd, buf, sizeof(buf));
}

int main(int argc, char *argv[]) {
    int ret;

    struct joystick *ds4_js = key_alloc(DS4_AXIS_SIZE, DS4_BTN_SIZE, &ds4_key_config);
    struct joystick *switch_js = key_alloc(NS_AXIS_SIZE, NS_BTN_SIZE, &ns_key_config);
    ret = js_setup_input(js_node, ds4_js, NULL);
    if (ret < 0) {
        fprintf(stderr, "js_setup_input : %s\n", strerror(errno));
        goto done;
    }
    ret = js_setup_output(sw_fd, switch_js, on_event);
    if (ret < 0) {
        fprintf(stderr, "js_setup_output : %s\n", strerror(errno));
        goto done;
    }
    ret = js_loop();
    goto done;

    done:
    js_release();
    key_free(ds4_js);
    key_free(switch_js);
    return ret;
}
