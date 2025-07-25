#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include <inttypes.h>
// WiFi configuration
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define WIFI_MAXIMUM_RETRY 5

// Default WiFi credentials (can be changed via web interface)
#define DEFAULT_WIFI_SSID "GymRFID_Config"
#define DEFAULT_WIFI_PASS "gym123456"

// Event bits
extern const int WIFI_CONNECTED_BIT;
extern const int WIFI_FAIL_BIT;

/**
 * @brief Initialize WiFi manager
 * @param event_group Event group for WiFi events
 */
void wifi_manager_init(EventGroupHandle_t event_group);

/**
 * @brief Connect to WiFi network
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(const char* ssid, const char* password);

/**
 * @brief Start WiFi AP mode for configuration
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_start_ap(void);

/**
 * @brief Stop WiFi AP mode
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_stop_ap(void);

/**
 * @brief Get current WiFi status
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get current IP address
 * @param ip_str Buffer to store IP string
 * @param len Buffer length
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char* ip_str, size_t len);

#endif // WIFI_MANAGER_H

