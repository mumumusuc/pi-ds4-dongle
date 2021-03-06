#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <byteswap.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sbc/sbc.h>
#include <alsa/asoundlib.h>
#include "ds4-report.h"

#define REPORT_SIZE         550
#define SBC_DEF_FREQ        32000
#define FRAME_PER_PACKET    4

static int interrupted = 0;
static uint freq = SBC_DEF_FREQ;
static int hid_fd;
static sbc_t sbc;
static uint16_t frame;
static uint8_t report_buf[REPORT_SIZE];
static snd_pcm_t *cap_handle;
static snd_pcm_uframes_t frames;
static size_t sbc_frame_size;
static uint8_t *pcm_buffer, *sbc_buffer;
static char *hid_node = "/dev/hidraw2";

static struct sbc_config {
    uint8_t subbands;
    uint8_t bitpool;
    uint8_t blocks;
    int joint;
    int dualchannel;
    int snr;
    int msbc;
} config = {
        .bitpool = 25,
        .subbands = 8,
        .blocks = 16,
        .dualchannel = 1,
        .joint = 0,
        .msbc = 0,
        .snr = 0,
};

static int init_pcm_capture(snd_pcm_t **handle, snd_pcm_uframes_t *frame, uint *freq);

static int init_pcm_playback(snd_pcm_t **handle, snd_pcm_uframes_t *frame, uint *freq);

static void init_sbc_encoder(sbc_t *sbc, struct sbc_config *config, uint32_t sample_rate, uint8_t channels);

static int read_audio_data(uint8_t *data, size_t count);

static void read_pcm_sound(void *);

static void on_time() {
    size_t inc;
    ssize_t report_len;
    report_len = ds4_report_17(frame, &inc, report_buf, sbc_buffer);
    write(hid_fd, report_buf, report_len);
    /* read next data */
    int ret = read_audio_data(sbc_buffer, FRAME_PER_PACKET);
    if (ret < 0) {
        printf("audio lag.\n");
        return;;
    }
    frame += inc;
    if (frame > 0xffff)
        frame = 0;
}

int main(int argc, char *argv[]) {

    if (argc > 1)
        hid_node = argv[1];
    hid_fd = open(hid_node, O_RDWR);
    if (hid_fd < 0) {
        fprintf(stderr, "open node[%s] failed : %s\n", hid_node, strerror(errno));
        exit(-1);
    }

    int ret;
    size_t packet_interval;
    ret = init_pcm_capture(&cap_handle, &frames, &freq);
    if (ret < 0) {
        perror("init_pcm_playback");
        return ret;
    }
    init_sbc_encoder(&sbc, &config, freq, 2);
    /* 8*16/32 = 4ms  */
    packet_interval = sbc.subbands * sbc.blocks / (freq / 1000) * FRAME_PER_PACKET;
    /* 512 bytes */
    sbc_frame_size = sbc_get_codesize(&sbc);
    pcm_buffer = (uint8_t *) malloc(sbc_frame_size);
    sbc_buffer = (uint8_t *) malloc(sbc_frame_size * FRAME_PER_PACKET);

    struct sigaction act;
    act.sa_handler = on_time;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
    struct itimerval val;
    val.it_value.tv_sec = 0;
    val.it_value.tv_usec = packet_interval * 1000;
    val.it_interval = val.it_value;
    setitimer(ITIMER_REAL, &val, NULL);
    while (!interrupted) {
        sleep(10);
    }

    free(pcm_buffer);
    free(sbc_buffer);
    sbc_finish(&sbc);
    snd_pcm_drain(cap_handle);
    snd_pcm_close(cap_handle);
    return 0;
}

static ssize_t read_sbc_frames(uint8_t *output, size_t out_size, size_t frames) {
    int ret;
    ssize_t sbc_len, sbc_encoded;
    size_t dt_cap, dt_encode;
    struct timeval start, end;
    size_t sbc_code_size = sbc_get_codesize(&sbc);
    gettimeofday(&start, NULL);
    ret = snd_pcm_readi(cap_handle, pcm_buffer, frames);
    if (ret < 0) {
        perror("snd_pcm_readi");
        return ret;
    }
    gettimeofday(&end, NULL);
    dt_cap = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    gettimeofday(&start, NULL);
    sbc_len = sbc_encode(&sbc, pcm_buffer, sbc_code_size, output, out_size, &sbc_encoded);
    if (sbc_len != sbc_code_size || sbc_encoded <= 0) {
        fprintf(stderr, "sbc_encode fail, sbc_len=%ld, encoded=%lu\n", sbc_len, (unsigned long) sbc_encoded);
        return -EINVAL;
    }
    gettimeofday(&end, NULL);
    dt_encode = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    printf("capture %d frames, use %lu us; sbc encode %ld bytes, use %lu us\n", ret, dt_cap, sbc_encoded, dt_encode);
    return sbc_encoded;
}

static int init_pcm_capture(snd_pcm_t **handle, snd_pcm_uframes_t *frame, uint *freq) {
    int ret;
    snd_pcm_hw_params_t *params = NULL;
    ret = snd_pcm_open(handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_malloc(&params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_any(*handle, params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_S16_BE);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_channels(*handle, params, 2);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_rate_near(*handle, params, freq, 0);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params(*handle, params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_get_period_size(params, frame, 0);
    if (ret < 0) goto done;

    goto done;

    done:
    snd_pcm_hw_params_free(params);
    return ret;
}

static int init_pcm_playback(snd_pcm_t **handle, snd_pcm_uframes_t *frame, uint *freq) {
    int ret;
    snd_pcm_hw_params_t *params = NULL;
    ret = snd_pcm_open(handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_malloc(&params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_any(*handle, params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_S16_BE);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_rate_near(*handle, params, freq, 0);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_set_channels(*handle, params, 2);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params(*handle, params);
    if (ret < 0) goto done;
    ret = snd_pcm_hw_params_get_period_size(params, frame, 0);
    if (ret < 0) goto done;

    goto done;

    done:
    snd_pcm_hw_params_free(params);
    return ret;
}

static void init_sbc_encoder(sbc_t *sbc, struct sbc_config *config, uint32_t sample_rate, uint8_t channels) {
    if (!config->msbc) {
        sbc_init(sbc, 0L);
        switch (sample_rate) {
            case 16000:
                sbc->frequency = SBC_FREQ_16000;
                break;
            case 32000:
                sbc->frequency = SBC_FREQ_32000;
                break;
            case 44100:
                sbc->frequency = SBC_FREQ_44100;
                break;
            case 48000:
                sbc->frequency = SBC_FREQ_48000;
                break;
        }
        sbc->subbands = config->subbands == 4 ? SBC_SB_4 : SBC_SB_8;
        if (channels == 1) {
            sbc->mode = SBC_MODE_MONO;
            if (config->joint || config->dualchannel) {
                fprintf(stderr, "Audio is mono but joint or dualchannel mode has been specified\n");
                return;
            }
        } else if (config->joint && !config->dualchannel)
            sbc->mode = SBC_MODE_JOINT_STEREO;
        else if (!config->joint && config->dualchannel)
            sbc->mode = SBC_MODE_DUAL_CHANNEL;
        else if (!config->joint && !config->dualchannel)
            sbc->mode = SBC_MODE_STEREO;
        else {
            fprintf(stderr, "Both joint and dualchannel mode have been specified\n");
            return;
        }
        sbc->endian = SBC_BE;
        sbc->bitpool = config->bitpool;
        sbc->allocation = config->snr ? SBC_AM_SNR : SBC_AM_LOUDNESS;
        sbc->blocks = config->blocks == 4 ? SBC_BLK_4 :
                      config->blocks == 8 ? SBC_BLK_8 :
                      config->blocks == 12 ? SBC_BLK_12 : SBC_BLK_16;
    } else {
        if (sample_rate != 16000 || channels != 1 || channels != 1) {
            fprintf(stderr, "mSBC requires 16 bits, 16kHz, mono input\n");
            return;
        }
        sbc_init_msbc(sbc, 0);
        sbc->endian = SBC_BE;
    }
    printf("encoding with rate %d, %d blocks, "
           "%d subbands, %d bits, allocation method %s, "
           "and mode %s\n", sample_rate, config->blocks, config->subbands, config->bitpool,
           sbc->allocation == SBC_AM_SNR ? "SNR" : "LOUDNESS",
           sbc->mode == SBC_MODE_MONO ? "MONO" :
           sbc->mode == SBC_MODE_STEREO ? "STEREO" : "JOINTSTEREO");
}

__always_inline static int read_audio_data(uint8_t *data, size_t count) {
    int ret = 0;
    for (int i = 0; i < count; i++) {
        ret = read_sbc_frames(data + ret * i, sbc_frame_size, sbc_frame_size / 4);
        if (ret < 0)
            return ret;
    }
    return ret * count;
}