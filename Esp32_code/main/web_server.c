#include "web_server.h"
#include "wifi_manager.h"
#include "rfid_manager.h"
#include "user_manager.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include <string.h>
#include <sys/stat.h>
#include "esp_timer.h"
#include <inttypes.h>
static const char *TAG = "WEB_SERVER";

static httpd_handle_t s_server = NULL;

// Forward declarations
static esp_err_t root_handler(httpd_req_t *req);
static esp_err_t api_status_handler(httpd_req_t *req);
static esp_err_t api_cards_handler(httpd_req_t *req);
static esp_err_t api_users_handler(httpd_req_t *req);
static esp_err_t api_users_post_handler(httpd_req_t *req);
static esp_err_t api_users_put_handler(httpd_req_t *req);
static esp_err_t api_users_delete_handler(httpd_req_t *req);
static esp_err_t api_access_log_handler(httpd_req_t *req);
static esp_err_t api_config_handler(httpd_req_t *req);
static esp_err_t api_config_post_handler(httpd_req_t *req);
static esp_err_t static_file_handler(httpd_req_t *req);

// Helper functions
static esp_err_t set_cors_headers(httpd_req_t *req);
static esp_err_t send_json_response(httpd_req_t *req, cJSON *json, int status_code);
static esp_err_t send_error_response(httpd_req_t *req, int status_code, const char *message);
static const char* get_content_type(const char* filename);

esp_err_t web_server_init(void)
{
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_uri_handlers = 20;
    config.max_resp_headers = 8;
    config.stack_size = 8192;
    
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register URI handlers
    
    // Root handler
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &root_uri);
    
    // API handlers
    httpd_uri_t api_status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = api_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_status_uri);
    
    httpd_uri_t api_cards_uri = {
        .uri = "/api/cards",
        .method = HTTP_GET,
        .handler = api_cards_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_cards_uri);
    
    httpd_uri_t api_users_get_uri = {
        .uri = "/api/users",
        .method = HTTP_GET,
        .handler = api_users_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_users_get_uri);
    
    httpd_uri_t api_users_post_uri = {
        .uri = "/api/users",
        .method = HTTP_POST,
        .handler = api_users_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_users_post_uri);
    
    httpd_uri_t api_users_put_uri = {
        .uri = "/api/users/*",
        .method = HTTP_PUT,
        .handler = api_users_put_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_users_put_uri);
    
    httpd_uri_t api_users_delete_uri = {
        .uri = "/api/users/*",
        .method = HTTP_DELETE,
        .handler = api_users_delete_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_users_delete_uri);
    
    httpd_uri_t api_access_log_uri = {
        .uri = "/api/access-log",
        .method = HTTP_GET,
        .handler = api_access_log_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_access_log_uri);
    
    httpd_uri_t api_config_get_uri = {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = api_config_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_config_get_uri);
    
    httpd_uri_t api_config_post_uri = {
        .uri = "/api/config",
        .method = HTTP_POST,
        .handler = api_config_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &api_config_post_uri);
    
    // Static file handler (catch-all)
    httpd_uri_t static_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = static_file_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &static_uri);
    
    ESP_LOGI(TAG, "Web server started on port %d", WEB_SERVER_PORT);
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (s_server == NULL) {
        return ESP_OK;
    }
    
    esp_err_t ret = httpd_stop(s_server);
    s_server = NULL;
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Web server stopped");
    } else {
        ESP_LOGE(TAG, "Failed to stop web server: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

bool web_server_is_running(void)
{
    return (s_server != NULL);
}

httpd_handle_t web_server_get_handle(void)
{
    return s_server;
}

static esp_err_t set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    return ESP_OK;
}

static esp_err_t send_json_response(httpd_req_t *req, cJSON *json, int status_code)
{
    set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");
    
    if (status_code != 200) {
        char status_str[512];
        snprintf(status_str, sizeof(status_str), "%d", status_code);
        httpd_resp_set_status(req, status_str);
    }
    
    const char *json_string = cJSON_Print(json);
    esp_err_t ret = httpd_resp_send(req, json_string, strlen(json_string));
    
    free((void *)json_string);
    cJSON_Delete(json);
    
    return ret;
}

static esp_err_t send_error_response(httpd_req_t *req, int status_code, const char *message)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", message);
    cJSON_AddNumberToObject(json, "code", status_code);
    
    return send_json_response(req, json, status_code);
}

static const char* get_content_type(const char* filename)
{
    if (strstr(filename, ".html")) return "text/html";
    if (strstr(filename, ".css")) return "text/css";
    if (strstr(filename, ".js")) return "application/javascript";
    if (strstr(filename, ".json")) return "application/json";
    if (strstr(filename, ".png")) return "image/png";
    if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) return "image/jpeg";
    if (strstr(filename, ".gif")) return "image/gif";
    if (strstr(filename, ".svg")) return "image/svg+xml";
    if (strstr(filename, ".ico")) return "image/x-icon";
    return "text/plain";
}

static esp_err_t root_handler(httpd_req_t *req)
{
    return static_file_handler(req);
}

static esp_err_t api_status_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    
    // System status
    cJSON_AddStringToObject(json, "device", "ESP32 Gym RFID System");
    cJSON_AddStringToObject(json, "version", "1.0.0");
    cJSON_AddBoolToObject(json, "wifi_connected", wifi_manager_is_connected());
    cJSON_AddBoolToObject(json, "rfid_connected", rfid_manager_is_connected());
    cJSON_AddStringToObject(json, "rfid_status", rfid_manager_get_status());
    
    // Get IP address if connected
    if (wifi_manager_is_connected()) {
        char ip_str[16];
        if (wifi_manager_get_ip(ip_str, sizeof(ip_str)) == ESP_OK) {
            cJSON_AddStringToObject(json, "ip_address", ip_str);
        }
    }
    
    // System uptime
    cJSON_AddNumberToObject(json, "uptime", esp_timer_get_time() / 1000000);
    
    return send_json_response(req, json, 200);
}

static esp_err_t api_cards_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    
    rfid_card_data_t card_data;
    if (rfid_manager_get_last_card(&card_data) == ESP_OK) {
        cJSON_AddStringToObject(json, "uid", card_data.uid_str);
        cJSON_AddNumberToObject(json, "length", card_data.uid_len);
        cJSON_AddNumberToObject(json, "timestamp", card_data.timestamp);
        cJSON_AddBoolToObject(json, "valid", card_data.is_valid);
    } else {
        cJSON_AddStringToObject(json, "status", "No card detected");
    }
    
    return send_json_response(req, json, 200);
}

static esp_err_t api_users_handler(httpd_req_t *req)
{
    gym_user_t users[50]; // Maximum 50 users
    uint32_t count = 0;
    
    esp_err_t ret = user_manager_get_all_users(users, 50, &count);
    if (ret != ESP_OK) {
        return send_error_response(req, 500, "Failed to get users");
    }
    
    cJSON *json = cJSON_CreateArray();
    
    for (uint32_t i = 0; i < count; i++) {
        cJSON *user_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(user_json, "id", users[i].id);
        cJSON_AddStringToObject(user_json, "name", users[i].name);
        cJSON_AddStringToObject(user_json, "rfid_uid", users[i].rfid_uid);
        cJSON_AddNumberToObject(user_json, "access_level", users[i].access_level);
        cJSON_AddStringToObject(user_json, "access_level_name", user_manager_get_access_level_name(users[i].access_level));
        cJSON_AddBoolToObject(user_json, "is_active", users[i].is_active);
        cJSON_AddNumberToObject(user_json, "created_time", users[i].created_time);
        cJSON_AddNumberToObject(user_json, "last_access", users[i].last_access);
        
        cJSON_AddItemToArray(json, user_json);
    }
    
    return send_json_response(req, json, 200);
}

static esp_err_t api_users_post_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        return send_error_response(req, 400, "Invalid request body");
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        return send_error_response(req, 400, "Invalid JSON");
    }
    
    cJSON *name_json = cJSON_GetObjectItem(json, "name");
    cJSON *rfid_uid_json = cJSON_GetObjectItem(json, "rfid_uid");
    cJSON *access_level_json = cJSON_GetObjectItem(json, "access_level");
    
    if (!cJSON_IsString(name_json) || !cJSON_IsString(rfid_uid_json) || !cJSON_IsNumber(access_level_json)) {
        cJSON_Delete(json);
        return send_error_response(req, 400, "Missing required fields");
    }
    
    const char *name = name_json->valuestring;
    const char *rfid_uid = rfid_uid_json->valuestring;
    uint8_t access_level = (uint8_t)access_level_json->valueint;
    
    uint32_t user_id;
    esp_err_t result = user_manager_create_user(name, rfid_uid, access_level, &user_id);
    
    cJSON_Delete(json);
    
    if (result == ESP_OK) {
        cJSON *response = cJSON_CreateObject();
        cJSON_AddNumberToObject(response, "id", user_id);
        cJSON_AddStringToObject(response, "message", "User created successfully");
        return send_json_response(req, response, 201);
    } else if (result == ESP_ERR_INVALID_STATE) {
        return send_error_response(req, 409, "RFID UID already exists");
    } else {
        return send_error_response(req, 500, "Failed to create user");
    }
}

static esp_err_t api_users_put_handler(httpd_req_t *req)
{
    // Extract user ID from URI
    const char *uri = req->uri;
    const char *id_str = strrchr(uri, '/');
    if (id_str == NULL) {
        return send_error_response(req, 400, "Invalid user ID");
    }
    id_str++; // Skip the '/'
    
    uint32_t user_id = atoi(id_str);
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        return send_error_response(req, 400, "Invalid request body");
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        return send_error_response(req, 400, "Invalid JSON");
    }
    
    cJSON *name_json = cJSON_GetObjectItem(json, "name");
    cJSON *rfid_uid_json = cJSON_GetObjectItem(json, "rfid_uid");
    cJSON *access_level_json = cJSON_GetObjectItem(json, "access_level");
    
    const char *name = cJSON_IsString(name_json) ? name_json->valuestring : NULL;
    const char *rfid_uid = cJSON_IsString(rfid_uid_json) ? rfid_uid_json->valuestring : NULL;
    uint8_t access_level = cJSON_IsNumber(access_level_json) ? (uint8_t)access_level_json->valueint : 0;
    
    esp_err_t result = user_manager_update_user(user_id, name, rfid_uid, access_level);
    
    cJSON_Delete(json);
    
    if (result == ESP_OK) {
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "message", "User updated successfully");
        return send_json_response(req, response, 200);
    } else if (result == ESP_ERR_NOT_FOUND) {
        return send_error_response(req, 404, "User not found");
    } else if (result == ESP_ERR_INVALID_STATE) {
        return send_error_response(req, 409, "RFID UID already exists");
    } else {
        return send_error_response(req, 500, "Failed to update user");
    }
}

static esp_err_t api_users_delete_handler(httpd_req_t *req)
{
    // Extract user ID from URI
    const char *uri = req->uri;
    const char *id_str = strrchr(uri, '/');
    if (id_str == NULL) {
        return send_error_response(req, 400, "Invalid user ID");
    }
    id_str++; // Skip the '/'
    
    uint32_t user_id = atoi(id_str);
    
    esp_err_t result = user_manager_delete_user(user_id);
    
    if (result == ESP_OK) {
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "message", "User deleted successfully");
        return send_json_response(req, response, 200);
    } else if (result == ESP_ERR_NOT_FOUND) {
        return send_error_response(req, 404, "User not found");
    } else {
        return send_error_response(req, 500, "Failed to delete user");
    }
}

static esp_err_t api_access_log_handler(httpd_req_t *req)
{
    access_log_t logs[100]; // Maximum 100 recent logs
    uint32_t count = 0;
    
    esp_err_t ret = storage_manager_get_recent_logs(logs, 100, &count);
    if (ret != ESP_OK) {
        return send_error_response(req, 500, "Failed to get access logs");
    }
    
    cJSON *json = cJSON_CreateArray();
    
    for (uint32_t i = 0; i < count; i++) {
        cJSON *log_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(log_json, "id", logs[i].id);
        cJSON_AddNumberToObject(log_json, "user_id", logs[i].user_id);
        cJSON_AddStringToObject(log_json, "rfid_uid", logs[i].rfid_uid);
        cJSON_AddNumberToObject(log_json, "timestamp", logs[i].timestamp);
        cJSON_AddBoolToObject(log_json, "access_granted", logs[i].access_granted);
        cJSON_AddStringToObject(log_json, "location", logs[i].location);
        
        cJSON_AddItemToArray(json, log_json);
    }
    
    return send_json_response(req, json, 200);
}

static esp_err_t api_config_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    
    // WiFi configuration
    char ssid[32];
    char password[64];
    if (storage_manager_get_wifi_credentials(ssid, password) == ESP_OK) {
        cJSON_AddStringToObject(json, "wifi_ssid", ssid);
        // Don't send password for security
    }
    
    cJSON_AddBoolToObject(json, "wifi_connected", wifi_manager_is_connected());
    cJSON_AddBoolToObject(json, "rfid_scanning", rfid_manager_is_scanning());
    
    return send_json_response(req, json, 200);
}

static esp_err_t api_config_post_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        return send_error_response(req, 400, "Invalid request body");
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        return send_error_response(req, 400, "Invalid JSON");
    }
    
    cJSON *wifi_ssid_json = cJSON_GetObjectItem(json, "wifi_ssid");
    cJSON *wifi_password_json = cJSON_GetObjectItem(json, "wifi_password");
    
    if (cJSON_IsString(wifi_ssid_json) && cJSON_IsString(wifi_password_json)) {
        const char *ssid = wifi_ssid_json->valuestring;
        const char *password = wifi_password_json->valuestring;
        
        esp_err_t result = wifi_manager_connect(ssid, password);
        if (result == ESP_OK) {
            cJSON_Delete(json);
            cJSON *response = cJSON_CreateObject();
            cJSON_AddStringToObject(response, "message", "WiFi configuration updated");
            return send_json_response(req, response, 200);
        }
    }
    
    cJSON_Delete(json);
    return send_error_response(req, 400, "Invalid configuration");
}

static esp_err_t static_file_handler(httpd_req_t *req)
{
    char filepath[1024];
    const char *filename = req->uri;
    
    // Default to index.html for root
    if (strcmp(filename, "/") == 0) {
        filename = "/index.html";
    }
   
    snprintf(filepath, sizeof(filepath), "/spiffs%s", filename);
    
    // Check if file exists
    struct stat file_stat;
    if (stat(filepath, &file_stat) != 0) {
        // File not found, serve index.html for SPA routing
        snprintf(filepath, sizeof(filepath), "/spiffs/index.html");
        if (stat(filepath, &file_stat) != 0) {
            return send_error_response(req, 404, "File not found");
        }
    }
    
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return send_error_response(req, 500, "Failed to open file");
    }
    
    set_cors_headers(req);
    httpd_resp_set_type(req, get_content_type(filename));
    
    char buffer[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            fclose(file);
            return ESP_FAIL;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0); // End response
    
    return ESP_OK;
}

