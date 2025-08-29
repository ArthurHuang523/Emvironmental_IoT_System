idf_component_register(
    SRC_DIRS 
        "." 
        "APP" 
    INCLUDE_DIRS 
        "."
        "APP" 
    REQUIRES BSP LVGL esp_wifi esp_event esp_netif nvs_flash lwip log mqtt app_update esp_http_client
)
