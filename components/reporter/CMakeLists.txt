idf_component_register(
    SRCS "esp_hid_gap.c" "reporter.c"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash esp_hid
)
