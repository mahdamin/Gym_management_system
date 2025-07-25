#include "storage_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
static const char *TAG = "STORAGE_MANAGER";

static nvs_handle_t s_nvs_handle;

esp_err_t storage_manager_init(void)
{
    esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set default admin credentials if not exists
    char admin_user[64];
    size_t required_size = sizeof(admin_user);
    ret = nvs_get_str(s_nvs_handle, KEY_ADMIN_USER, admin_user, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Setting default admin credentials");
        storage_manager_set_admin_credentials("admin", "gym123456");
    }
    
    ESP_LOGI(TAG, "Storage manager initialized");
    return ESP_OK;
}

esp_err_t storage_manager_set_wifi_credentials(const char* ssid, const char* password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = nvs_set_str(s_nvs_handle, KEY_WIFI_SSID, ssid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting WiFi SSID: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_set_str(s_nvs_handle, KEY_WIFI_PASS, password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting WiFi password: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing WiFi credentials: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t storage_manager_get_wifi_credentials(char* ssid, char* password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t required_size = 32;
    esp_err_t ret = nvs_get_str(s_nvs_handle, KEY_WIFI_SSID, ssid, &required_size);
    if (ret != ESP_OK) {
        return ret;
    }
    
    required_size = 64;
    ret = nvs_get_str(s_nvs_handle, KEY_WIFI_PASS, password, &required_size);
    return ret;
}

esp_err_t storage_manager_set_admin_credentials(const char* username, const char* password)
{
    if (username == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = nvs_set_str(s_nvs_handle, KEY_ADMIN_USER, username);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting admin username: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_set_str(s_nvs_handle, KEY_ADMIN_PASS, password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error setting admin password: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing admin credentials: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

bool storage_manager_verify_admin_credentials(const char* username, const char* password)
{
    if (username == NULL || password == NULL) {
        return false;
    }
    
    char stored_user[64];
    char stored_pass[64];
    size_t required_size = sizeof(stored_user);
    
    esp_err_t ret = nvs_get_str(s_nvs_handle, KEY_ADMIN_USER, stored_user, &required_size);
    if (ret != ESP_OK) {
        return false;
    }
    
    required_size = sizeof(stored_pass);
    ret = nvs_get_str(s_nvs_handle, KEY_ADMIN_PASS, stored_pass, &required_size);
    if (ret != ESP_OK) {
        return false;
    }
    
    return (strcmp(username, stored_user) == 0 && strcmp(password, stored_pass) == 0);
}

esp_err_t storage_manager_save_user(const gym_user_t* user)
{
    if (user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char key[32];
    snprintf(key, sizeof(key), "user_%u", user->id);
    
    esp_err_t ret = nvs_set_blob(s_nvs_handle, key, user, sizeof(gym_user_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error saving user %u: %s", user->id, esp_err_to_name(ret));
        return ret;
    }
    
    // Update user count
    uint32_t user_count = 0;
    size_t required_size = sizeof(user_count);
    nvs_get_blob(s_nvs_handle, KEY_USER_COUNT, &user_count, &required_size);
    
    if (user->id >= user_count) {
        user_count = user->id + 1;
        nvs_set_blob(s_nvs_handle, KEY_USER_COUNT, &user_count, sizeof(user_count));
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing user data: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t storage_manager_get_user(uint32_t user_id, gym_user_t* user)
{
    if (user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char key[32];
    snprintf(key, sizeof(key), "user_%u", user_id);
    
    size_t required_size = sizeof(gym_user_t);
    esp_err_t ret = nvs_get_blob(s_nvs_handle, key, user, &required_size);
    
    return ret;
}

esp_err_t storage_manager_get_user_by_rfid(const char* rfid_uid, gym_user_t* user)
{
    if (rfid_uid == NULL || user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t user_count = 0;
    size_t required_size = sizeof(user_count);
    esp_err_t ret = nvs_get_blob(s_nvs_handle, KEY_USER_COUNT, &user_count, &required_size);
    if (ret != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }
    
    for (uint32_t i = 0; i < user_count; i++) {
        gym_user_t temp_user;
        ret = storage_manager_get_user(i, &temp_user);
        if (ret == ESP_OK && temp_user.is_active && strcmp(temp_user.rfid_uid, rfid_uid) == 0) {
            memcpy(user, &temp_user, sizeof(gym_user_t));
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t storage_manager_delete_user(uint32_t user_id)
{
    char key[32];
    snprintf(key, sizeof(key), "user_%u", user_id);
    
    esp_err_t ret = nvs_erase_key(s_nvs_handle, key);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error deleting user %u: %s", user_id, esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing user deletion: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t storage_manager_get_all_users(gym_user_t* users, uint32_t max_users, uint32_t* count)
{
    if (users == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t user_count = 0;
    size_t required_size = sizeof(user_count);
    esp_err_t ret = nvs_get_blob(s_nvs_handle, KEY_USER_COUNT, &user_count, &required_size);
    if (ret != ESP_OK) {
        *count = 0;
        return ESP_OK;
    }
    
    uint32_t found_count = 0;
    for (uint32_t i = 0; i < user_count && found_count < max_users; i++) {
        gym_user_t temp_user;
        ret = storage_manager_get_user(i, &temp_user);
        if (ret == ESP_OK && temp_user.is_active) {
            memcpy(&users[found_count], &temp_user, sizeof(gym_user_t));
            found_count++;
        }
    }
    
    *count = found_count;
    return ESP_OK;
}

esp_err_t storage_manager_add_access_log(const access_log_t* log)
{
    if (log == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Get current log count
    uint32_t log_count = 0;
    size_t required_size = sizeof(log_count);
    nvs_get_blob(s_nvs_handle, KEY_ACCESS_LOG_COUNT, &log_count, &required_size);
    
    // Save log entry
    char key[32];
    snprintf(key, sizeof(key), "log_%u", log_count);
    
    esp_err_t ret = nvs_set_blob(s_nvs_handle, key, log, sizeof(access_log_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error saving access log: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Update log count
    log_count++;
    ret = nvs_set_blob(s_nvs_handle, KEY_ACCESS_LOG_COUNT, &log_count, sizeof(log_count));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error updating log count: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing access log: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t storage_manager_get_recent_logs(access_log_t* logs, uint32_t max_logs, uint32_t* count)
{
    if (logs == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t log_count = 0;
    size_t required_size = sizeof(log_count);
    esp_err_t ret = nvs_get_blob(s_nvs_handle, KEY_ACCESS_LOG_COUNT, &log_count, &required_size);
    if (ret != ESP_OK) {
        *count = 0;
        return ESP_OK;
    }
    
    uint32_t start_index = (log_count > max_logs) ? (log_count - max_logs) : 0;
    uint32_t found_count = 0;
    
    for (uint32_t i = start_index; i < log_count && found_count < max_logs; i++) {
        char key[32];
        snprintf(key, sizeof(key), "log_%u", i);
        
        access_log_t temp_log;
        required_size = sizeof(access_log_t);
        ret = nvs_get_blob(s_nvs_handle, key, &temp_log, &required_size);
        if (ret == ESP_OK) {
            memcpy(&logs[found_count], &temp_log, sizeof(access_log_t));
            found_count++;
        }
    }
    
    *count = found_count;
    return ESP_OK;
}

esp_err_t storage_manager_clear_access_logs(void)
{
    uint32_t log_count = 0;
    size_t required_size = sizeof(log_count);
    esp_err_t ret = nvs_get_blob(s_nvs_handle, KEY_ACCESS_LOG_COUNT, &log_count, &required_size);
    if (ret != ESP_OK) {
        return ESP_OK; // No logs to clear
    }
    
    // Delete all log entries
    for (uint32_t i = 0; i < log_count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "log_%u", i);
        nvs_erase_key(s_nvs_handle, key);
    }
    
    // Reset log count
    log_count = 0;
    ret = nvs_set_blob(s_nvs_handle, KEY_ACCESS_LOG_COUNT, &log_count, sizeof(log_count));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error resetting log count: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing log clear: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

uint32_t storage_manager_get_next_user_id(void)
{
    uint32_t user_count = 0;
    size_t required_size = sizeof(user_count);
    nvs_get_blob(s_nvs_handle, KEY_USER_COUNT, &user_count, &required_size);
    return user_count;
}

