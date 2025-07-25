#include "rfid_manager.h"
#include "user_manager.h"
#include "storage_manager.h"
#include "rc522.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include "esp_timer.h"
#include <inttypes.h>
static const char *TAG = "RFID_MANAGER";

static rc522_handle_t s_rc522_handle;
static rfid_event_callback_t s_event_callback = NULL;
static rfid_card_data_t s_last_card_data = {0};
static bool s_is_connected = false;
static bool s_is_scanning = false;
static TaskHandle_t s_scan_task_handle = NULL;

// Forward declarations
static void rfid_scan_task(void* pvParameters);
static void rfid_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

esp_err_t rfid_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing RFID manager");
    
    // Configure RC522
    rc522_config_t config = RC522_DEFAULT_CONFIG(RC522_SPI_SDA_PIN, RC522_RST_PIN);
    config.spi.mosi_gpio_num = RC522_SPI_MOSI_PIN;
    config.spi.miso_gpio_num = RC522_SPI_MISO_PIN;
    config.spi.sck_gpio_num = RC522_SPI_SCK_PIN;
    
    // Create RC522 handle
    esp_err_t ret = rc522_create(&config, &s_rc522_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RC522 handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register event handler
    ret = rc522_register_events(s_rc522_handle, RC522_EVENT_ANY, rfid_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register RC522 events: %s", esp_err_to_name(ret));
        rc522_destroy(s_rc522_handle);
        return ret;
    }
    
    // Start RC522
    ret = rc522_start(s_rc522_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start RC522: %s", esp_err_to_name(ret));
        rc522_destroy(s_rc522_handle);
        return ret;
    }
    
    s_is_connected = true;
    
    // Start scanning task
    rfid_manager_start_scanning();
    
    ESP_LOGI(TAG, "RFID manager initialized successfully");
    return ESP_OK;
}

void rfid_manager_set_callback(rfid_event_callback_t callback)
{
    s_event_callback = callback;
}

esp_err_t rfid_manager_get_last_card(rfid_card_data_t* card_data)
{
    if (card_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_last_card_data.is_valid) {
        return ESP_ERR_NOT_FOUND;
    }
    
    memcpy(card_data, &s_last_card_data, sizeof(rfid_card_data_t));
    return ESP_OK;
}

bool rfid_manager_is_connected(void)
{
    return s_is_connected;
}

const char* rfid_manager_get_status(void)
{
    if (!s_is_connected) {
        return "Disconnected";
    } else if (s_is_scanning) {
        return "Scanning";
    } else {
        return "Connected";
    }
}

void rfid_manager_uid_to_string(const uint8_t* uid, uint8_t uid_len, char* uid_str, size_t str_len)
{
    if (uid == NULL || uid_str == NULL || str_len == 0) {
        return;
    }
    
    char* ptr = uid_str;
    size_t remaining = str_len;
    
    for (int i = 0; i < uid_len && remaining > 2; i++) {
        int written = snprintf(ptr, remaining, "%02X", uid[i]);
        if (written > 0 && written < remaining) {
            ptr += written;
            remaining -= written;
            
            if (i < uid_len - 1 && remaining > 1) {
                *ptr++ = ':';
                remaining--;
            }
        }
    }
    *ptr = '\0';
}

esp_err_t rfid_manager_start_scanning(void)
{
    if (s_is_scanning) {
        return ESP_OK; // Already scanning
    }
    
    if (!s_is_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    BaseType_t ret = xTaskCreate(rfid_scan_task, "rfid_scan", 4096, NULL, 5, &s_scan_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RFID scan task");
        return ESP_FAIL;
    }
    
    s_is_scanning = true;
    ESP_LOGI(TAG, "RFID scanning started");
    return ESP_OK;
}

esp_err_t rfid_manager_stop_scanning(void)
{
    if (!s_is_scanning) {
        return ESP_OK; // Already stopped
    }
    
    s_is_scanning = false;
    
    if (s_scan_task_handle != NULL) {
        vTaskDelete(s_scan_task_handle);
        s_scan_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "RFID scanning stopped");
    return ESP_OK;
}

bool rfid_manager_is_scanning(void)
{
    return s_is_scanning;
}

static void rfid_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base != RC522_EVENTS) {
        return;
    }
    
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    
    switch (event_id) {
        case RC522_EVENT_PICC_DETECTED:
            ESP_LOGI(TAG, "PICC detected");
            break;
            
        case RC522_EVENT_PICC_REMOVED:
            ESP_LOGI(TAG, "PICC removed");
            break;
            
        case RC522_EVENT_PICC_STATE_CHANGED:
            if (data->picc_state_changed.new_state == RC522_PICC_STATE_ACTIVE) {
                // Card is active, read UID
                rc522_picc_t* picc = data->picc_state_changed.picc;
                
                // Update last card data
                s_last_card_data.uid_len = picc->uid.size;
                memcpy(s_last_card_data.uid, picc->uid.value, picc->uid.size);
                
                // Convert UID to string
                rfid_manager_uid_to_string(picc->uid.value, picc->uid.size, 
                                         s_last_card_data.uid_str, sizeof(s_last_card_data.uid_str));
                
                // Set timestamp
                struct timeval tv;
                gettimeofday(&tv, NULL);
                s_last_card_data.timestamp = tv.tv_sec;
                s_last_card_data.is_valid = true;
                
                ESP_LOGI(TAG, "Card detected: %s", s_last_card_data.uid_str);
                
                // Process access control
                gym_user_t user;
                esp_err_t ret = user_manager_authenticate_rfid(s_last_card_data.uid_str, &user);
                
                access_log_t log = {0};
                log.timestamp = s_last_card_data.timestamp;
                strncpy(log.rfid_uid, s_last_card_data.uid_str, sizeof(log.rfid_uid) - 1);
                strncpy(log.location, "Main Entrance", sizeof(log.location) - 1);
                
                if (ret == ESP_OK) {
                    // Access granted
                    log.user_id = user.id;
                    log.access_granted = true;
                    user_manager_update_last_access(user.id);
                    ESP_LOGI(TAG, "Access granted for user: %s", user.name);
                } else {
                    // Access denied
                    log.user_id = 0;
                    log.access_granted = false;
                    ESP_LOGW(TAG, "Access denied for RFID: %s", s_last_card_data.uid_str);
                }
                
                // Save access log
                storage_manager_add_access_log(&log);
                
                // Call event callback if set
                if (s_event_callback != NULL) {
                    s_event_callback(&s_last_card_data);
                }
            }
            break;
            
        default:
            break;
    }
}

static void rfid_scan_task(void* pvParameters)
{
    ESP_LOGI(TAG, "RFID scan task started");
    
    while (s_is_scanning) {
        // The RC522 library handles scanning automatically through events
        // This task just needs to keep running to maintain the scanning state
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "RFID scan task ended");
    vTaskDelete(NULL);
}

