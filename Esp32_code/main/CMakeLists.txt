idf_component_register(SRCS "main.c" "wifi_manager.c" "web_server.c" "rfid_manager.c" "user_manager.c" "storage_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES "nvs_flash" "log" "esp_wifi" "esp_netif" "esp_timer" "esp_event" "esp_http_server" "driver" "spi_flash" "spiffs" "json")

add_compile_options(-Wno-error=format)
