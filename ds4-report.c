/* Please read this webpage for more details */
/* https://www.psdevwiki.com/ps4/DS4-BT */
#include <memory.h>
#include <stdio.h>
#include "crc32.h"
#include "ds4-report.h"

#define DEBUG   0

#define REPORT_SIZE     550
#define FRAME_SIZE      112
#define HEADER_SPEAKER  0x02
#define HEADER_HEADJACK 0x24
#define DS4_REPORT_14_SIZE  270
#define DS4_REPORT_15_SIZE  334
#define DS4_REPORT_17_SIZE  462
#define DS4_REPORT_18_SIZE  526
#define DS4_REPORT_19_SIZE  547

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

#define put_unaligned_le16(value, buffer)   \
    (buffer)[0] = 0xff & ((value) >> 0);    \
    (buffer)[1] = 0xff & ((value) >> 8);


#define put_unaligned_le32(value, buffer)   \
    (buffer)[0] = 0xff & ((value) >> 0);    \
    (buffer)[1] = 0xff & ((value) >> 8);    \
    (buffer)[2] = 0xff & ((value) >> 16);   \
    (buffer)[3] = 0xff & ((value) >> 24);

__always_inline static void print_report(const uint8_t *buf, size_t size) {
#if DEBUG
    for (int i = 0; i < size; i++)
        printf("%02x ", buf[i]);
    puts("\n");
#endif
}

ssize_t ds4_report_14(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data) {
    int offset = 6;
    *written = 2;
    memset(report_buf, 0, DS4_REPORT_14_SIZE);
    report_buf[0] = 0x14;
    report_buf[1] = 0x40;
    report_buf[2] = 0xa0;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;
    memcpy(report_buf + offset, sbc_data, (*written) * FRAME_SIZE);
    /* crc32 */
    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_14_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_14_SIZE - 4]);
    print_report(report_buf, DS4_REPORT_14_SIZE);
    return DS4_REPORT_14_SIZE;
}

ssize_t ds4_report_15(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data) {
    int offset = 81;
    *written = 2;
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
    memcpy(report_buf + offset, sbc_data, (*written) * FRAME_SIZE);
    /* crc32 */
    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_15_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_15_SIZE - 4]);
    print_report(report_buf, DS4_REPORT_15_SIZE);
    return DS4_REPORT_15_SIZE;
}

ssize_t ds4_report_17(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data) {
    int offset = 6;
    *written = 4;
    memset(report_buf, 0, DS4_REPORT_17_SIZE);
    report_buf[0] = 0x17;
    report_buf[1] = 0x40;
    report_buf[2] = 0xa0;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;
    memcpy(report_buf + offset, sbc_data, (*written) * FRAME_SIZE);
    /* crc32 */
    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_17_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_17_SIZE - 4]);
    print_report(report_buf, DS4_REPORT_17_SIZE);
    return DS4_REPORT_17_SIZE;
}

ssize_t ds4_report_18(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data) {
    int offset = 6;
    *written = 4;
    memset(report_buf, 0, DS4_REPORT_18_SIZE);
    report_buf[0] = 0x18;
    report_buf[1] = 0x48;
    report_buf[2] = 0xa1;
    put_unaligned_le16(frame, &report_buf[3]);
    report_buf[5] = header;
    memcpy(report_buf + offset, sbc_data, (*written) * FRAME_SIZE);
    /* crc32 */
    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_18_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_18_SIZE - 4]);
    print_report(report_buf, DS4_REPORT_18_SIZE);
    return DS4_REPORT_18_SIZE;
}

ssize_t ds4_report_19(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data) {
    int offset = 82;
    *written = 4;
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
    memcpy(report_buf + offset, sbc_data, (*written) * FRAME_SIZE);
    /* crc32 */
    uint32_t crc = 0xFFFFFFFF;
    crc = crc32_le(crc, &output, 1);
    crc = crc32(crc, report_buf, DS4_REPORT_19_SIZE - 4);
    put_unaligned_le32(crc, &report_buf[DS4_REPORT_19_SIZE - 4]);
    print_report(report_buf, DS4_REPORT_19_SIZE);
    return DS4_REPORT_19_SIZE;
}


