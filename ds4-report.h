/* Please read this webpage for more details */
/* https://www.psdevwiki.com/ps4/DS4-BT */

#ifndef DS4_SBC_DS4_REPORT_H
#define DS4_SBC_DS4_REPORT_H

#include <stdint.h>
#include <unistd.h>

ssize_t ds4_report_11(uint8_t *report_buf);

/*
 * @params:
 *      frame       : current frames
 *      written     : writen frames
 *      report_buf  : report buffer
 *      sbc_data    : sbc data
 * @return
 *      report size, negative when error.
 */
ssize_t ds4_report_14(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data);

ssize_t ds4_report_15(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data);

ssize_t ds4_report_17(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data);

ssize_t ds4_report_18(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data);

ssize_t ds4_report_19(uint16_t frame, size_t *written, uint8_t *report_buf, uint8_t *sbc_data);

#endif //DS4_SBC_DS4_REPORT_H
