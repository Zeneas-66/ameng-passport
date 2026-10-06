#include "ameng_audio.h"

#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>

#define SAMPLE_RATE 16000
#define CHUNK 256

typedef struct {
    uint8_t variant;
} audio_cmd_t;

static const char *TAG = "ameng_audio";
static QueueHandle_t s_queue;
static uint8_t s_volume = 70;

static int16_t triangle(uint32_t phase)
{
    uint16_t p = (uint16_t)(phase >> 16);
    int32_t v = (p < 32768U) ? ((int32_t)p - 16384)
                             : (49152 - (int32_t)p);
    return (int16_t)v;
}

static void play_one(uint8_t variant)
{
    static const uint16_t start_hz[] = {650, 520, 760, 580, 690, 470};
    static const uint16_t end_hz[]   = {420, 760, 500, 390, 820, 610};
    static const uint16_t dur_ms[]   = {420, 560, 360, 720, 500, 620};

    variant %= 6;
    const uint32_t total = (uint32_t)SAMPLE_RATE * dur_ms[variant] / 1000U;
    uint32_t phase1 = 0;
    uint32_t phase2 = 0;
    int16_t pcm[CHUNK];

    if (bsp_audio_set_format(SAMPLE_RATE, 16, 1) != ESP_OK) return;
    bsp_audio_set_volume(s_volume);

    uint32_t done = 0;
    while (done < total) {
        uint32_t n = total - done;
        if (n > CHUNK) n = CHUNK;

        for (uint32_t i = 0; i < n; ++i) {
            uint32_t pos = done + i;
            uint32_t hz = start_hz[variant] +
                ((int32_t)(end_hz[variant] - start_hz[variant]) * (int32_t)pos) /
                (int32_t)total;

            /* Fast integer oscillator: two triangle partials plus a strong
             * attack/decay envelope gives a recognizable short "meow" without
             * keeping PCM samples in RAM. */
            uint32_t inc1 = (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE);
            uint32_t inc2 = (uint32_t)(((uint64_t)(hz * 2U + 37U) << 32) / SAMPLE_RATE);
            phase1 += inc1;
            phase2 += inc2;

            int32_t env;
            uint32_t attack = total / 8U;
            uint32_t release = total / 3U;
            if (pos < attack) {
                env = (int32_t)(pos * 1000U / (attack ? attack : 1U));
            } else if (pos > total - release) {
                env = (int32_t)((total - pos) * 1000U / (release ? release : 1U));
            } else {
                env = 1000;
            }

            int32_t s = (int32_t)triangle(phase1) * 3 +
                        (int32_t)triangle(phase2);
            s = s * env / 1000 / 5;
            pcm[i] = (int16_t)s;
        }

        if (bsp_audio_write(pcm, (size_t)n * sizeof(int16_t)) != ESP_OK) {
            ESP_LOGW(TAG, "meow playback failed");
            return;
        }
        done += n;
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
