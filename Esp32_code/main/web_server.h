#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include <inttypes.h>
// Server configuration
#define WEB_SERVER_PORT 80
#define MAX_FILE_SIZE 4096

/**
 * @brief Initialize web server
 * @return ESP_OK on success
 */
esp_err_t web_server_init(void);

/**
 * @brief Stop web server
 * @return ESP_OK on success
 */
esp_err_t web_server_stop(void);

/**
 * @brief Check if web server is running
 * @return true if running, false otherwise
 */
bool web_server_is_running(void);

/**
 * @brief Get server handle
 * @return Server handle or NULL if not running
 */
httpd_handle_t web_server_get_handle(void);

#endif // WEB_SERVER_H

