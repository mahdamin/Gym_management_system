#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "storage_manager.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "esp_timer.h"
#include <inttypes.h>
// Access levels
#define ACCESS_LEVEL_MEMBER 1
#define ACCESS_LEVEL_TRAINER 2
#define ACCESS_LEVEL_ADMIN 3

/**
 * @brief Initialize user manager
 * @return ESP_OK on success
 */
esp_err_t user_manager_init(void);

/**
 * @brief Create new user
 * @param name User name
 * @param rfid_uid RFID UID string
 * @param access_level Access level
 * @param user_id Output parameter for new user ID
 * @return ESP_OK on success
 */
esp_err_t user_manager_create_user(const char* name, const char* rfid_uid, uint8_t access_level, uint32_t* user_id);

/**
 * @brief Update existing user
 * @param user_id User ID
 * @param name New name (NULL to keep current)
 * @param rfid_uid New RFID UID (NULL to keep current)
 * @param access_level New access level (0 to keep current)
 * @return ESP_OK on success
 */
esp_err_t user_manager_update_user(uint32_t user_id, const char* name, const char* rfid_uid, uint8_t access_level);

/**
 * @brief Delete user
 * @param user_id User ID
 * @return ESP_OK on success
 */
esp_err_t user_manager_delete_user(uint32_t user_id);

/**
 * @brief Authenticate user by RFID
 * @param rfid_uid RFID UID string
 * @param user Output parameter for user data
 * @return ESP_OK if authenticated, ESP_ERR_NOT_FOUND if not found
 */
esp_err_t user_manager_authenticate_rfid(const char* rfid_uid, gym_user_t* user);

/**
 * @brief Update user last access time
 * @param user_id User ID
 * @return ESP_OK on success
 */
esp_err_t user_manager_update_last_access(uint32_t user_id);

/**
 * @brief Get all users
 * @param users Buffer for user array
 * @param max_users Maximum number of users
 * @param count Actual number of users retrieved
 * @return ESP_OK on success
 */
esp_err_t user_manager_get_all_users(gym_user_t* users, uint32_t max_users, uint32_t* count);

/**
 * @brief Check if RFID UID is already registered
 * @param rfid_uid RFID UID string
 * @return true if exists, false otherwise
 */
bool user_manager_rfid_exists(const char* rfid_uid);

/**
 * @brief Get user by ID
 * @param user_id User ID
 * @param user Output parameter for user data
 * @return ESP_OK on success
 */
esp_err_t user_manager_get_user(uint32_t user_id, gym_user_t* user);

/**
 * @brief Validate access level
 * @param access_level Access level to validate
 * @return true if valid, false otherwise
 */
bool user_manager_is_valid_access_level(uint8_t access_level);

/**
 * @brief Get access level name
 * @param access_level Access level
 * @return Access level name string
 */
const char* user_manager_get_access_level_name(uint8_t access_level);

#endif // USER_MANAGER_H

