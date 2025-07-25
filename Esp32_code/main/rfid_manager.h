#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
// RC522 Pin definitions (adjust according to your wiring)
#define RC522_SPI_MOSI_PIN         23  // MOSI -> GPIO23
#define RC522_SPI_MISO_PIN         19  // MISO -> GPIO19
#define RC522_SPI_SCK_PIN          18  // SCK (Clock) -> GPIO18
#define RC522_SPI_SDA_PIN          5   // SDA (Chip Select) -> GPIO5
#define RC522_RST_PIN              22  // RST -> GPIO22

// RFID card data structure
typedef struct {
    uint8_t uid[10];
    uint8_t uid_len;
    char uid_str[32];
    uint64_t timestamp;
    bool is_valid;
} rfid_card_data_t;

// RFID event callback function type
typedef void (*rfid_event_callback_t)(const rfid_card_data_t* card_data);

/**
 * @brief Initialize RFID manager
 * @return ESP_OK on success
 */
esp_err_t rfid_manager_init(void);

/**
 * @brief Set RFID event callback
 * @param callback Callback function for RFID events
 */
void rfid_manager_set_callback(rfid_event_callback_t callback);

/**
 * @brief Get last detected card data
 * @param card_data Buffer for card data
 * @return ESP_OK if valid data available
 */
esp_err_t rfid_manager_get_last_card(rfid_card_data_t* card_data);

/**
 * @brief Check if RFID reader is connected and working
 * @return true if connected, false otherwise
 */
bool rfid_manager_is_connected(void);

/**
 * @brief Get RFID reader status
 * @return Status string
 */
const char* rfid_manager_get_status(void);

/**
 * @brief Convert UID bytes to string
 * @param uid UID bytes
 * @param uid_len UID length
 * @param uid_str Output string buffer
 * @param str_len String buffer length
 */
void rfid_manager_uid_to_string(const uint8_t* uid, uint8_t uid_len, char* uid_str, size_t str_len);

/**
 * @brief Start RFID scanning task
 * @return ESP_OK on success
 */
esp_err_t rfid_manager_start_scanning(void);

/**
 * @brief Stop RFID scanning task
 * @return ESP_OK on success
 */
esp_err_t rfid_manager_stop_scanning(void);

/**
 * @brief Check if scanning is active
 * @return true if scanning, false otherwise
 */
bool rfid_manager_is_scanning(void);

#endif // RFID_MANAGER_H

