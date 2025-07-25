#include "user_manager.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
static const char *TAG = "USER_MANAGER";

esp_err_t user_manager_init(void)
{
    ESP_LOGI(TAG, "User manager initialized");
    return ESP_OK;
}

esp_err_t user_manager_create_user(const char* name, const char* rfid_uid, uint8_t access_level, uint32_t* user_id)
{
    if (name == NULL || rfid_uid == NULL || user_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!user_manager_is_valid_access_level(access_level)) {
        ESP_LOGE(TAG, "Invalid access level: %d", access_level);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if RFID UID already exists
    if (user_manager_rfid_exists(rfid_uid)) {
        ESP_LOGE(TAG, "RFID UID already exists: %s", rfid_uid);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Get next user ID
    uint32_t new_user_id = storage_manager_get_next_user_id();
    
    // Create user structure
    gym_user_t user = {0};
    user.id = new_user_id;
    strncpy(user.name, name, sizeof(user.name) - 1);
    strncpy(user.rfid_uid, rfid_uid, sizeof(user.rfid_uid) - 1);
    user.access_level = access_level;
    user.is_active = true;
    
    // Set creation time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    user.created_time = tv.tv_sec;
    user.last_access = 0;
    
    // Save user
    esp_err_t ret = storage_manager_save_user(&user);
    if (ret == ESP_OK) {
        *user_id = new_user_id;
        ESP_LOGI(TAG, "Created user: %s (ID: %u, RFID: %s)", name, new_user_id, rfid_uid);
    } else {
        ESP_LOGE(TAG, "Failed to create user: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t user_manager_update_user(uint32_t user_id, const char* name, const char* rfid_uid, uint8_t access_level)
{
    gym_user_t user;
    esp_err_t ret = storage_manager_get_user(user_id, &user);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "User not found: %u", user_id);
        return ret;
    }
    
    // Update name if provided
    if (name != NULL) {
        strncpy(user.name, name, sizeof(user.name) - 1);
        user.name[sizeof(user.name) - 1] = '\0';
    }
    
    // Update RFID UID if provided
    if (rfid_uid != NULL) {
        // Check if new RFID UID already exists (but not for this user)
        gym_user_t existing_user;
        if (storage_manager_get_user_by_rfid(rfid_uid, &existing_user) == ESP_OK && existing_user.id != user_id) {
            ESP_LOGE(TAG, "RFID UID already exists: %s", rfid_uid);
            return ESP_ERR_INVALID_STATE;
        }
        strncpy(user.rfid_uid, rfid_uid, sizeof(user.rfid_uid) - 1);
        user.rfid_uid[sizeof(user.rfid_uid) - 1] = '\0';
    }
    
    // Update access level if provided
    if (access_level != 0) {
        if (!user_manager_is_valid_access_level(access_level)) {
            ESP_LOGE(TAG, "Invalid access level: %d", access_level);
            return ESP_ERR_INVALID_ARG;
        }
        user.access_level = access_level;
    }
    
    // Save updated user
    ret = storage_manager_save_user(&user);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Updated user: %u", user_id);
    } else {
        ESP_LOGE(TAG, "Failed to update user: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t user_manager_delete_user(uint32_t user_id)
{
    // First check if user exists
    gym_user_t user;
    esp_err_t ret = storage_manager_get_user(user_id, &user);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "User not found: %u", user_id);
        return ret;
    }
    
    // Mark user as inactive instead of deleting
    user.is_active = false;
    ret = storage_manager_save_user(&user);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Deleted user: %u", user_id);
    } else {
        ESP_LOGE(TAG, "Failed to delete user: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t user_manager_authenticate_rfid(const char* rfid_uid, gym_user_t* user)
{
    if (rfid_uid == NULL || user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = storage_manager_get_user_by_rfid(rfid_uid, user);
    if (ret == ESP_OK) {
        if (user->is_active) {
            ESP_LOGI(TAG, "Authenticated user: %s (ID: %u)", user->name, user->id);
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Inactive user attempted access: %s", user->name);
            return ESP_ERR_INVALID_STATE;
        }
    } else {
        ESP_LOGW(TAG, "Unknown RFID UID: %s", rfid_uid);
        return ESP_ERR_NOT_FOUND;
    }
}

esp_err_t user_manager_update_last_access(uint32_t user_id)
{
    gym_user_t user;
    esp_err_t ret = storage_manager_get_user(user_id, &user);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Update last access time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    user.last_access = tv.tv_sec;
    
    return storage_manager_save_user(&user);
}

esp_err_t user_manager_get_all_users(gym_user_t* users, uint32_t max_users, uint32_t* count)
{
    return storage_manager_get_all_users(users, max_users, count);
}

bool user_manager_rfid_exists(const char* rfid_uid)
{
    if (rfid_uid == NULL) {
        return false;
    }
    
    gym_user_t user;
    return (storage_manager_get_user_by_rfid(rfid_uid, &user) == ESP_OK);
}

esp_err_t user_manager_get_user(uint32_t user_id, gym_user_t* user)
{
    return storage_manager_get_user(user_id, user);
}

bool user_manager_is_valid_access_level(uint8_t access_level)
{
    return (access_level >= ACCESS_LEVEL_MEMBER && access_level <= ACCESS_LEVEL_ADMIN);
}

const char* user_manager_get_access_level_name(uint8_t access_level)
{
    switch (access_level) {
        case ACCESS_LEVEL_MEMBER:
            return "Member";
        case ACCESS_LEVEL_TRAINER:
            return "Trainer";
        case ACCESS_LEVEL_ADMIN:
            return "Admin";
        default:
            return "Unknown";
    }
}

