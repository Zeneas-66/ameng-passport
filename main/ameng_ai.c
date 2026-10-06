#include "ameng_ai.h"

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "ameng_ai";
#define WIFI_CONNECTED_BIT BIT0
#define AI_RESPONSE_MAX 2048

static EventGroupHandle_t s_wifi_events;
static bool s_initialized;
static bool s_enabled;

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

esp_err_t ameng_ai_init(void)
{
    if (s_initialized) return ESP_OK;
    s_initialized = true;

#if !CONFIG_AMENG_AI_ENABLE
    s_enabled = false;
    return ESP_OK;
#else
    if (CONFIG_AMENG_AI_API_KEY[0] == '\0' ||
        CONFIG_AMENG_WIFI_SSID[0] == '\0' ||
        CONFIG_AMENG_AI_ENDPOINT[0] == '\0') {
        ESP_LOGW(TAG, "AI disabled: configure Wi-Fi and AI credentials in menuconfig");
        s_enabled = false;
        return ESP_OK;
    }

    s_enabled = true;
    s_wifi_events = xEventGroupCreate();
    if (!s_wifi_events) return ESP_ERR_NO_MEM;

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (!netif) return ESP_ERR_NO_MEM;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;
    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, NULL);
    if (err != ESP_OK) return err;
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event, NULL);
    if (err != ESP_OK) return err;

    wifi_config_t wifi = {0};
    snprintf((char *)wifi.sta.ssid, sizeof(wifi.sta.ssid), "%s", CONFIG_AMENG_WIFI_SSID);
    snprintf((char *)wifi.sta.password, sizeof(wifi.sta.password), "%s", CONFIG_AMENG_WIFI_PASSWORD);
    wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) err = esp_wifi_set_config(WIFI_IF_STA, &wifi);
    if (err == ESP_OK) err = esp_wifi_start();
    return err;
#endif
}

bool ameng_ai_enabled(void)
{
    return s_enabled;
}

bool ameng_ai_online(void)
{
    if (!s_enabled || !s_wifi_events) return false;
    return (xEventGroupGetBits(s_wifi_events) & WIFI_CONNECTED_BIT) != 0;
}

typedef struct {
    char *buf;
    size_t cap;
    size_t used;
} response_buf_t;

static esp_err_t http_event(esp_http_client_event_t *evt)
{
    response_buf_t *r = evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && r && evt->data_len > 0) {
        size_t room = r->cap > r->used ? r->cap - r->used - 1 : 0;
        size_t copy = (size_t)evt->data_len < room ? (size_t)evt->data_len : room;
        if (copy) {
            memcpy(r->buf + r->used, evt->data, copy);
            r->used += copy;
            r->buf[r->used] = '\0';
        }
    }
    return ESP_OK;
}

esp_err_t ameng_ai_generate(const ameng_state_t *state, uint64_t now_s,
                            uint8_t local_hour, char *out, size_t out_size)
{
#if !CONFIG_AMENG_AI_ENABLE
    (void)state; (void)now_s; (void)local_hour; (void)out; (void)out_size;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!state || !out || out_size == 0) return ESP_ERR_INVALID_ARG;
    if (!ameng_ai_online()) return ESP_ERR_INVALID_STATE;

    char facts[768];
    if (ameng_state_ai_context(state, now_s, local_hour, facts, sizeof(facts)) < 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    cJSON_AddStringToObject(root, "model", CONFIG_AMENG_AI_MODEL);
    cJSON_AddNumberToObject(root, "temperature", 0.8);
    cJSON_AddNumberToObject(root, "max_tokens", 40);

    cJSON *system = cJSON_CreateObject();
    cJSON_AddStringToObject(system, "role", "system");
    cJSON_AddStringToObject(system, "content",
        "You are Ameng, a large long-haired white-and-yellow cat: proud, quiet, food-loving, "
        "aloof-looking but deeply affectionate. Reply as Ameng in one short ASCII-only sentence, "
        "max 36 characters. Never invent memories. Never contradict AMENG_FACTS.");
    cJSON_AddItemToArray(messages, system);

    cJSON *user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "role", "user");
    cJSON_AddStringToObject(user, "content", facts);
    cJSON_AddItemToArray(messages, user);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) return ESP_ERR_NO_MEM;

    char response[AI_RESPONSE_MAX] = {0};
    response_buf_t rb = {.buf = response, .cap = sizeof(response), .used = 0};
    esp_http_client_config_t cfg = {
        .url = CONFIG_AMENG_AI_ENDPOINT,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event,
        .user_data = &rb,
        .timeout_ms = 12000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        free(payload);
        return ESP_ERR_NO_MEM;
    }

    char auth[320];
    snprintf(auth, sizeof(auth), "Bearer %s", CONFIG_AMENG_AI_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    free(payload);
    if (err != ESP_OK) return err;
    if (status < 200 || status >= 300) return ESP_FAIL;

    cJSON *parsed = cJSON_Parse(response);
    if (!parsed) return ESP_ERR_INVALID_RESPONSE;
    cJSON *choices = cJSON_GetObjectItem(parsed, "choices");
    cJSON *choice = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : NULL;
    cJSON *message = choice ? cJSON_GetObjectItem(choice, "message") : NULL;
    cJSON *content = message ? cJSON_GetObjectItem(message, "content") : NULL;
    if (!cJSON_IsString(content) || !content->valuestring) {
        cJSON_Delete(parsed);
        return ESP_ERR_INVALID_RESPONSE;
    }
    snprintf(out, out_size, "%s", content->valuestring);
    cJSON_Delete(parsed);
    return ESP_OK;
#endif
}
