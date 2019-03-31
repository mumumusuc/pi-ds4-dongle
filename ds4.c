#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include "crc32.h"

#define DEBUG           1
#define USE_TIMER       0

#define REPORT_SIZE     550
#define FRAME_SIZE      112
#define HEADER_SPEAKER  0x02
#define HEADER_HEADJACK 0x24

static char *hid_node = "/dev/hidraw2";
static uint8_t report_buf[REPORT_SIZE];

static uint8_t input = 0xa1;
static uint8_t output = 0xa2;
static uint8_t header = HEADER_HEADJACK;
static uint8_t rumble_weak = 0;
static uint8_t rumble_strong = 0;
static uint8_t r = 0;
static uint8_t g = 0;
static uint8_t b = 0;
static uint8_t volume_mic = 0;
static uint8_t volume_speaker = 0xa0;
static uint8_t volume_l = 0xff;
static uint8_t volume_r = 0xff;
static uint8_t unk2 = 0;
static uint8_t unk3 = 0;
static uint8_t flash_on = 0;
static uint8_t flash_off = 0;

static int interrupted = 0;
static int audio_fd = -1;
static int hid_fd = -1;
/* interval = subbands * block / sample(kHZ) * frame_per_packet */
const size_t frame_interval = 8 * 16 / 16 * 1000 * 4;
static uint16_t frame;

/* See ds4-hid-report.txt */
#define DS4_REPORT_14_SIZE  270
#define DS4_REPORT_15_SIZE  334
#define DS4_REPORT_17_SIZE  462
#define DS4_REPORT_18_SIZE  526
#define DS4_REPORT_19_SIZE  547

static int check_hid_params(int);

static int _14_report(int, uint16_t);

static int _15_report(int, uint16_t);

static int _17_report(int, uint16_t);

static int _18_report(int, uint16_t);

static int _19_report(int, uint16_t);

static void on_time() {
    size_t inc;
    int dir = 1;
    struct timespec req, rem;
#if DEBUG | !USE_TIMER
    size_t dt;
    struct timeval start, end;
    gettimeofday(&start, NULL);
#endif
    inc = _17_report(hid_fd, frame);
#if DEBUG | !USE_TIMER
    gettimeofday(&end, NULL);
    dt = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    req.tv_sec = 0;
    req.tv_nsec = (frame_interval - dt) * 1000;
    printf("%lu packets cost %lu us"
           #if !USE_TIMER
           ", sleep %ld us"
           #endif
           "\n", inc, dt
           #if !USE_TIMER
            , frame_interval - dt
           #endif
    );
#if !USE_TIMER
    nanosleep(&req, &rem);
#endif
#endif
    frame += inc;
    if (frame > 0xffff)
        frame = 0;
    if (g == 0)
        dir = 1;
    else if (g == 255)
        dir = -1;
    g += dir;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        hid_node = argv[1];
        printf("set node : %s\n", hid_node);
    }

    hid_fd = open(hid_node, O_RDWR);
    if (hid_fd < 0) {
        fprintf(stderr, "open node[%s] failed : %s\n", hid_node, strerror(errno));
        exit(-1);
    }

#if DEBUG
    check_hid_params(hid_fd);
#endif

    const char *filename = argv[2];
    if (strcmp(filename, "-")) {
        audio_fd = open(filename, O_RDONLY);
        if (audio_fd < 0) {
            fprintf(stderr, "Can't open file %s: %s\n", filename, strerror(errno));
            exit(-1);
        }
    } else
        audio_fd = fileno(stdin);

#if USE_TIMER
    struct sigaction act;
    act.sa_handler = on_time;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGPROF, &act, NULL);
    struct itimerval val;
    val.it_value.tv_sec = 0;
    val.it_value.tv_usec = interval;
    val.it_interval = val.it_value;
    setitimer(ITIMER_PROF, &val, NULL);
#endif
    while (!interrupted) {
#if !USE_TIMER
        on_time();
#endif
    };

    close(audio_fd);
    close(hid_fd);
    return 0;
}

static int check_hid_params(int hid_fd) {
#define BUF_SIZE 256
    int ret;
    uint8_t buf[BUF_SIZE];
    struct hidraw_devinfo devinfo;
    struct hidraw_report_descriptor rpt_desc;
    /* get raw info */
    ret = ioctl(hid_fd, HIDIOCGRAWINFO, &devinfo);
    if (ret < 0) {
        perror("HIDIOCGRAWINFO");
        return ret;
    }
    printf("TYPE[%u], VID[%04X], PID[%04X]\n", devinfo.bustype, devinfo.vendor, devinfo.product);
    /* get desc size */
    ret = ioctl(hid_fd, HIDIOCGRDESCSIZE, &rpt_desc.size);
    if (ret < 0) {
        perror("HIDIOCGRDESCSIZE");
        return ret;
    }
    printf("Desc size: %u\n", rpt_desc.size);
    /* get desc */
    ret = ioctl(hid_fd, HIDIOCGRDESC, &rpt_desc);
    if (ret < 0) {
        perror("HIDIOCGRDESC");
        return ret;
    }
    printf("Report Descriptor:\n");
    for (int i = 0; i < rpt_desc.size; i++)
        printf("%02x ", rpt_desc.value[i]);
    puts("\n");
    /* get raw name */
    ret = ioctl(hid_fd, HIDIOCGRAWNAME(BUF_SIZE), buf);
    if (ret < 0) {
        perror("HIDIOCGRAWNAME");
        return ret;
    }
    printf("Raw Name: %s\n", buf);
    /* get physical location */

    if (ioctl(hid_fd, HIDIOCGRAWPHYS(BUF_SIZE), buf) < 0) {
        perror("HIDIOCGRAWPHYS");
        return ret;
    }
    printf("Raw Phys: %s\n", buf);
}

__always_inline static void put_unaligned_le16(uint32_t value, uint8_t *buffer) {
    buffer[0] = 0xff & (value >> 0);
    buffer[1] = 0xff & (value >> 8);
}

__always_inline static void put_unaligned_le32(uint32_t value, uint8_t *buffer) {
    buffer[0] = 0xff & (value >> 0);
    buffer[1] = 0xff & (value >> 8);
    buffer[2] = 0xff & (value >> 16);
    buffer[3] = 0xff & (value >> 24);
}

__always_inline static void print_report(const uint8_t *buf, size_t size) {
    /*
     for (int i = 0; i < size; i++)
        printf("%02x ", buf[i]);
    puts("\n");
*/
}

__always_inline static int read_audio_data(uint8_t *data, size_t count) {
    ssize_t size = 0;
    uint8_t *output = data;
    while (1) {
        output += read(audio_fd, &data[size], FRAME_SIZE * count - size);
        size = output - data;
        //printf("read[%ld]\n", size);
        if (output - data == FRAME_SIZE * count) {
            break;
        } else if (size < 0) {
            perror("read error");
            return size;
        } else if (size < FRAME_SIZE * count) {
            fprintf(stderr, "not enough data[%ld]\n", size);
            //return -ENODATA;
        }
    }
    return FRAME_SIZE;
}

static int _14_report(int fd, uint16_t frame) {
    int offset;
    memset(report_buf, 0, DS4_REPORT_14_SIZE);
    report_buf[0] = 0x14;
    report_buf[1] = 0x40;
    report_buf[2] = 0xa0;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;
    offset = 6;
    offset += read_audio_data(&report_buf[offset], 2);

    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_14_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_14_SIZE - 4]);
    write(fd, report_buf, DS4_REPORT_14_SIZE);
    return 2;
}

static int _15_report(int fd, uint16_t frame) {
    int offset;
    memset(report_buf, 0, DS4_REPORT_15_SIZE);
    report_buf[0] = 0x15;
    report_buf[1] = 0xc4;
    report_buf[2] = 0xa0;
    report_buf[3] = 0x07;
    report_buf[4] = 0x04;
    report_buf[5] = 0x00;
    report_buf[6] = rumble_weak;
    report_buf[7] = rumble_strong;
    report_buf[8] = r;
    report_buf[9] = g;
    report_buf[10] = b;
    report_buf[11] = flash_on;
    report_buf[12] = flash_off;
    report_buf[21] = volume_l;
    report_buf[22] = volume_r;
    report_buf[23] = unk2;
    report_buf[24] = volume_speaker;
    report_buf[25] = unk3;
    put_unaligned_le16(frame, &report_buf[78]);
    report_buf[80] = header;
    offset = 81;
    offset += read_audio_data(&report_buf[offset], 2);

    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_15_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_15_SIZE - 4]);
#if DEBUG
    print_report(report_buf, DS4_REPORT_15_SIZE);
#endif
    write(fd, report_buf, DS4_REPORT_15_SIZE);
    return 2;
}

static int _17_report(int fd, uint16_t frame) {
    int offset;
    memset(report_buf, 0, DS4_REPORT_17_SIZE);
    report_buf[0] = 0x17;
    report_buf[1] = 0x40;
    report_buf[2] = 0xa0;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;
    offset = 6;
    offset += read_audio_data(&report_buf[offset], 4);

    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_17_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_17_SIZE - 4]);
#if DEBUG
    print_report(report_buf, DS4_REPORT_17_SIZE);
#endif
    write(fd, report_buf, DS4_REPORT_17_SIZE);
    return 4;
}

static int _18_report(int fd, uint16_t frame) {
    int offset;
    memset(report_buf, 0, DS4_REPORT_18_SIZE);
    report_buf[0] = 0x18;
    report_buf[1] = 0x48;
    report_buf[2] = 0xa1;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;

    offset = 6;
    offset += read_audio_data(&report_buf[offset], 4);

    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_18_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_18_SIZE - 4]);
#if DEBUG
    print_report(report_buf, DS4_REPORT_18_SIZE);
#endif
    write(fd, report_buf, DS4_REPORT_18_SIZE);
    return 4;
}

static int _19_report(int fd, uint16_t frame) {
    int offset;
    memset(report_buf, 0, DS4_REPORT_19_SIZE);
    report_buf[0] = 0x19;
    report_buf[1] = 0xc0;
    report_buf[2] = 0xa0;
    report_buf[3] = 0xf3;
    report_buf[4] = 0x04;
    report_buf[5] = 0x00;
    report_buf[6] = rumble_weak;
    report_buf[7] = rumble_strong;
    report_buf[8] = r;
    report_buf[9] = g;
    report_buf[10] = b;
    report_buf[11] = flash_on;
    report_buf[12] = flash_off;
    report_buf[21] = volume_l;
    report_buf[22] = volume_r;
    report_buf[23] = unk2;
    report_buf[24] = volume_speaker;
    report_buf[25] = unk3;
    put_unaligned_le16(frame, &report_buf[79]);
    report_buf[81] = header;

    offset = 82;
    offset += read_audio_data(&report_buf[offset], 4);

    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_19_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_19_SIZE - 4]);
#if DEBUG
    print_report(report_buf, DS4_REPORT_19_SIZE);
#endif
    write(fd, report_buf, DS4_REPORT_19_SIZE);
    return 4;
}
