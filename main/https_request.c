
#include <string.h>
#include <stdio.h>
#include "display.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"

#define WEB_SERVER "api.intra.42.fr"
#define WEB_PORT 443
#define WEB_URL "https://api.intra.42.fr/oauth/token"

static const char *TAG = "example";

static const char *REQUEST = "GET https://api.intra.42.fr/v2/accreditations HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

#define CLIENT_ID "u-s4t2ud-e6da1b180d4c8bd503ab075940e9f70fb6355e0b93b0f439b0bb484e1c5c3554"
#define CLIENT_SECRET "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"

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

typedef struct s_access_token
{
    char    token[256];
    uint32_t created_at;
    uint16_t expires_at;
}               t_access_token;

static int send_request_for_token(struct esp_tls *tls)
{
    size_t      written_bytes;
    int         ret;
    const char  *post_request = \
        "POST https://api.intra.42.fr/oauth/token HTTP/1.0\r\n"
        "Host: api.intra.42.fr\r\n"
        "User-Agent: esp8266-freertos alekol-pocket\r\n"
        "Content-Length: 201\r\n"
        "\r\n"
        "grant_type=client_credentials&client_id="
        CLIENT_ID"&client_secret="CLIENT_SECRET;

    ESP_LOGI(TAG, "Request: %s", post_request);
    written_bytes = 0;
    while(written_bytes < strlen(post_request))
    {
        ret = esp_tls_conn_write(tls,
                                 post_request + written_bytes,
                                 strlen(post_request) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        }
        else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
            break ;
        }

    }
    return (ret);
}

static int  read_response(struct esp_tls *tls, char *response)
{
    bool    body_started;
    char    *body_ptr;
    char    buf[512];
    char    combined_buf[1024];
    int     bytes_read;

    body_started = false;
    bzero(combined_buf, sizeof(combined_buf));
    while (1)
    {
        bzero(buf, sizeof(buf));
        bytes_read = esp_tls_conn_read(tls, (char *)buf, sizeof(buf) - 1);

        if (bytes_read == ESP_TLS_ERR_SSL_WANT_READ
            || bytes_read == ESP_TLS_ERR_SSL_WANT_WRITE)
            continue;
        if (bytes_read <= 0)
            break;

        ESP_LOGD(TAG, "%d bytes read", bytes_read);

        strncat(combined_buf, buf, bytes_read);

        if (!body_started) {
            body_ptr = strstr(combined_buf, "\r\n\r\n");
            if (body_ptr != NULL) {
                body_ptr += 4;
                body_started = true;
            }
        }

        if (body_started) {
            strncat(response, body_ptr, strlen(body_ptr));
        }

        // If combined_buf exceeds its size, shift the data to the left
        if (strlen(combined_buf) > sizeof(combined_buf) - 1) {
            memmove(combined_buf, combined_buf + bytes_read, strlen(combined_buf) - bytes_read + 1);
        }
    }

    if(bytes_read == 0)
        ESP_LOGI(TAG, "connection closed");
    else if (bytes_read < 0)
        ESP_LOGE(TAG, "esp_tls_conn_read  bytes_readurned -0x%x", -bytes_read);
    return (bytes_read <= 0);
}

static char* extract_value(const char* key, char* response)
{
    char    *token;
    char    *value;

    token = strtok(response, ",");
    while (token != NULL)
    {
        value = strstr(token, key);
        if (value != NULL)
        {
            // skip the key and the colon separator
            value = value + strlen(key) + 2;

            // remove surrounding quotes
            if (*value == '\"')
                value++;
            if (*(value + strlen(value) - 1) == '\"')
                *(value + strlen(value) - 1) = '\0';

            return value;
        }
        token = strtok(NULL, ",");
    }
    return NULL;
}

static void    extract_access_token(char *response, t_access_token *access_token)
{
    // Remove surrounding '{}'
    response++;
    response[strlen(response) - 1] = 0;

    //access_token->token = strdup(extract_value("access_token", response));
    strncpy(access_token, extract_value("access_token", response), 256);
    access_token->expires_at = atoi(extract_value("expires_in", NULL));
    access_token->created_at = atoi(extract_value("created_at", NULL));
}

t_access_token  *get_access_token(void)
{
    char            response[256];
    t_access_token  *access_token;
    struct esp_tls  *tls;

    if (!(access_token = calloc(1, sizeof(*access_token)))) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return (NULL);
    }
    bzero(response, sizeof(response));
    
    esp_tls_cfg_t cfg = {
        .cacert_pem_buf  = server_root_cert_pem_start,
        .cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };

    if (!(tls = esp_tls_conn_http_new("https://api.intra.42.fr/oauth/token", &cfg))) {
        ESP_LOGE(TAG, "Connection failed...");
        return (NULL);
    }
    else
        ESP_LOGI(TAG, "Connection established...");

    if (send_request_for_token(tls) < 0) {
        esp_tls_conn_delete(tls);
        ESP_LOGE(TAG, "Failed to send request for token...");
        return (NULL);
    }

    ESP_LOGI(TAG, "Reading HTTP response...");
    if (read_response(tls, &response) < 0) {
        esp_tls_conn_delete(tls);
        ESP_LOGE(TAG, "Failed to read response...");
        return (NULL);
    }

    //Parse the response
    extract_access_token(response, access_token);

    esp_tls_conn_delete(tls);
    return (access_token);
}
