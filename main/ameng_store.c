#include "ameng_store.h"
#include "nvs.h"
#include "nvs_flash.h"

#define AMENG_MAGIC 0x414D454EU
#define AMENG_SCHEMA 2U

typedef struct {
    uint32_t magic;
    uint32_t schema;
    ameng_saved_t saved;
} ameng_blob_t;

esp_err_t ameng_store_init(void)
{
    return nvs_flash_init();
}

esp_err_t ameng_store_load(ameng_saved_t *out, bool *found)
{
    if (!out || !found) return ESP_ERR_INVALID_ARG;
    *found = false;

    nvs_handle_t h;
    esp_err_t err = nvs_open("ameng", NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;

    ameng_blob_t blob = {0};
    size_t size = sizeof(blob);
    err = nvs_get_blob(h, "state", &blob, &size);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    if (size != sizeof(blob) || blob.magic != AMENG_MAGIC ||
        blob.schema != AMENG_SCHEMA) {
        return ESP_ERR_INVALID_VERSION;
    }

    *out = blob.saved;
    *found = true;
    return ESP_OK;
}

esp_err_t ameng_store_save(const ameng_saved_t *saved)
{
    if (!saved) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open("ameng", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    const ameng_blob_t blob = {
        .magic = AMENG_MAGIC,
        .schema = AMENG_SCHEMA,
        .saved = *saved,
    };
    err = nvs_set_blob(h, "state", &blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}
