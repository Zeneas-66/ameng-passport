idf_component_register(
    SRCS "main.c"
         "ameng_pet.c"
         "ameng_store.c"
         "ameng_dialogue.c"
         "ameng_ai.c"
         "ameng_audio.c"
         "ameng_meow_sample.c"
         "ameng_ui.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_event esp_http_client esp_netif esp_timer esp_wifi json mbedtls nvs_flash
)
