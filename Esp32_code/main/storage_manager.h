#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
// Storage keys
#define STORAGE_NAMESPACE "gym_rfid"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"
#define KEY_ADMIN_USER "admin_user"
#define KEY_ADMIN_PASS "admin_pass"
#define KEY_USER_COUNT "user_count"
#define KEY_ACCESS_LOG_COUNT "log_count"

// User structure
typedef struct {
    uint32_t id;
    char name[64];
    char rfid_uid[32];
    uint8_t access_level;
    bool is_active;
    uint64_t created_time;
    uint64_t last_access;
} gym_user_t;

// Access log structure
typedef struct {
    uint32_t id;
    uint32_t user_id;
    char rfid_uid[32];
    uint64_t timestamp;
    bool access_granted;
    char location[32];
} access_log_t;

/**
 * @brief Initialize storage manager
 * @return ESP_OK on success
 */
esp_err_t storage_manager_init(void);

/**
 * @brief Set WiFi credentials
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return ESP_OK on success
 */
esp_err_t storage_manager_set_wifi_credentials(const char* ssid, const char* password);

/**
 * @brief Get WiFi credentials
 * @param ssid Buffer for SSID
 * @param password Buffer for password
 * @return ESP_OK on success
 */
esp_err_t storage_manager_get_wifi_credentials(char* ssid, char* password);

/**
 * @brief Set admin credentials
 * @param username Admin username
 * @param password Admin password
 * @return ESP_OK on success
 */
esp_err_t storage_manager_set_admin_credentials(const char* username, const char* password);

/**
 * @brief Verify admin credentials
 * @param username Admin username
 * @param password Admin password
 * @return true if valid, false otherwise
 */
bool storage_manager_verify_admin_credentials(const char* username, const char* password);

/**
 * @brief Add or update user
 * @param user User structure
 * @return ESP_OK on success
 */
esp_err_t storage_manager_save_user(const gym_user_t* user);

/**
 * @brief Get user by ID
 * @param user_id User ID
 * @param user Buffer for user data
 * @return ESP_OK on success
 */
esp_err_t storage_manager_get_user(uint32_t user_id, gym_user_t* user);

/**
 * @brief Get user by RFID UID
 * @param rfid_uid RFID UID string
 * @param user Buffer for user data
 * @return ESP_OK on success
 */
esp_err_t storage_manager_get_user_by_rfid(const char* rfid_uid, gym_user_t* user);

/**
 * @brief Delete user
 * @param user_id User ID
 * @return ESP_OK on success
 */
esp_err_t storage_manager_delete_user(uint32_t user_id);

/**
 * @brief Get all users
 * @param users Buffer for user array
 * @param max_users Maximum number of users to retrieve
 * @param count Actual number of users retrieved
 * @return ESP_OK on success
 */
esp_err_t storage_manager_get_all_users(gym_user_t* users, uint32_t max_users, uint32_t* count);

/**
 * @brief Add access log entry
 * @param log Access log entry
 * @return ESP_OK on success
 */
esp_err_t storage_manager_add_access_log(const access_log_t* log);

/**
 * @brief Get recent access logs
 * @param logs Buffer for log array
 * @param max_logs Maximum number of logs to retrieve
 * @param count Actual number of logs retrieved
 * @return ESP_OK on success
 */
esp_err_t storage_manager_get_recent_logs(access_log_t* logs, uint32_t max_logs, uint32_t* count);

/**
 * @brief Clear all access logs
 * @return ESP_OK on success
 */
esp_err_t storage_manager_clear_access_logs(void);

/**
 * @brief Get next available user ID
 * @return Next user ID
 */
uint32_t storage_manager_get_next_user_id(void);

#endif // STORAGE_MANAGER_H

