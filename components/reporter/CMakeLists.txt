idf_component_register(
    SRCS "esp_hidd_prf_api.c" "hid_dev.c" "hid_device_le_prf.c" "reporter.c"  
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash esp_hid input_matrix
)
