#include "ameng_audio.h"
#include "ameng_meow_sample.h"

#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>
#include <string.h>

#define CHUNK_BYTES 1024

typedef struct {
    uint8_t variant;
} audio_cmd_t;

static const char *TAG = "ameng_audio";
static QueueHandle_t s_queue;
static uint8_t s_volume = 70;

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool wav_pcm_info(const uint8_t *wav, size_t len,
                         const uint8_t **pcm, size_t *pcm_bytes,
                         uint32_t *sample_rate)
{
    if (!wav || len < 44 || memcmp(wav, "RIFF", 4) != 0 ||
        memcmp(wav + 8, "WAVE", 4) != 0) return false;

    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    const uint8_t *data = NULL;
    size_t data_len = 0;

    size_t pos = 12;
    while (pos + 8 <= len) {
        const uint8_t *chunk = wav + pos;
        uint32_t n = le32(chunk + 4);
        pos += 8;
        if (pos + n > len) return false;

        if (memcmp(chunk, "fmt ", 4) == 0 && n >= 16) {
            format = le16(wav + pos);
            channels = le16(wav + pos + 2);
            rate = le32(wav + pos + 4);
            bits = le16(wav + pos + 14);
        } else if (memcmp(chunk, "data", 4) == 0) {
            data = wav + pos;
            data_len = n;
        }
        pos += n + (n & 1U);
    }

    if (format != 1 || channels != 1 || bits != 16 || !data || !rate) {
        return false;
    }
    *pcm = data;
    *pcm_bytes = data_len & ~(size_t)1;
    *sample_rate = rate;
    return true;
}

static void play_one(uint8_t variant)
{
    const uint8_t *pcm = NULL;
    size_t pcm_bytes = 0;
    uint32_t base_rate = 0;
    if (!wav_pcm_info(ameng_meow_wav, ameng_meow_wav_size,
                      &pcm, &pcm_bytes, &base_rate)) {
        ESP_LOGE(TAG, "invalid embedded meow WAV");
        return;
    }

    /* A single real recording is played at subtly different sample rates.
     * This changes pitch/duration without synthetic oscillators, so repeated
     * responses do not sound exactly identical. */
    static const int8_t pct[] = {-7, -3, 0, 3, 6, -10};
    variant %= 6;
    uint32_t rate = (uint32_t)((int64_t)base_rate * (100 + pct[variant]) / 100);
    if (rate < 8000) rate = 8000;

    if (bsp_audio_set_format((int)rate, 16, 1) != ESP_OK) {
        ESP_LOGW(TAG, "audio format %lu Hz rejected", (unsigned long)rate);
        return;
    }
    bsp_audio_set_volume(s_volume);

    size_t pos = 0;
    while (pos < pcm_bytes) {
        size_t n = pcm_bytes - pos;
        if (n > CHUNK_BYTES) n = CHUNK_BYTES;
        n &= ~(size_t)1;
        if (!n) break;
        if (bsp_audio_write(pcm + pos, n) != ESP_OK) {
            ESP_LOGW(TAG, "real meow playback failed");
            return;
        }
        pos += n;
    }
}

static void worker(void *arg)
{
    (void)arg;
    audio_cmd_t cmd;
    for (;;) {
        if (xQueueReceive(s_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            play_one(cmd.variant);
        }
    }
}

esp_err_t ameng_audio_init(void)
{
    if (s_queue) return ESP_OK;

    esp_err_t err = bsp_audio_init();
    if (err != ESP_OK) return err;

    s_queue = xQueueCreate(3, sizeof(audio_cmd_t));
    if (!s_queue) return ESP_ERR_NO_MEM;

    if (xTaskCreate(worker, "ameng_audio", 4096, NULL, 4, NULL) != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void ameng_audio_set_volume(uint8_t volume)
{
    if (volume > 100) volume = 100;
    s_volume = volume;
}

void ameng_audio_meow(uint8_t variant)
{
    if (!s_queue) return;
    audio_cmd_t cmd = {.variant = variant};
    (void)xQueueSend(s_queue, &cmd, 0);
}
