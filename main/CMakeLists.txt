idf_component_register(
    SRCS "main.c"
         "ameng_pet.c"
         "ameng_store.c"
         "ameng_dialogue.c"
         "ameng_ai.c"
         "ameng_audio.c"
         "ameng_meow_sample.c"
         "ameng_meow_variants.c"
         "ameng_ui.c"
         "ameng_photo.c"
    INCLUDE_DIRS "."
    EMBED_FILES
         "assets/photos/living_1.rgb565"
         "assets/photos/living_2.rgb565"
         "assets/photos/living_3.rgb565"
         "assets/photos/workspace_1.rgb565"
         "assets/photos/workspace_2.rgb565"
         "assets/photos/workspace_3.rgb565"
         "assets/photos/dining_1.rgb565"
         "assets/photos/dining_2.rgb565"
         "assets/photos/dining_3.rgb565"
    REQUIRES bsp esp_event esp_http_client esp_netif esp_timer esp_wifi json mbedtls nvs_flash
)
