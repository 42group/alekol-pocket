#include <stdio.h>
#include <string.h>
#include <ssd1306/ssd1306.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include "display.h"
#include "wifi.h"
#include "logo.h"
#include <freertos/semphr.h>
#include "esp_log.h"

#define ACCESS_TOKEN_NVS_KEY "access_token"

typedef struct s_access_token
{
    char    token[256];
    uint32_t created_at;
    uint16_t expires_at;
}               t_access_token;

t_access_token  *get_access_token(void);

SemaphoreHandle_t   mutex;
t_display           *display;

void app_main(void)
{
    wifi_ap_record_t    ap_info;
    t_access_token      *access_token;
    nvs_handle          handle;
    esp_err_t           error;

    error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(error);

    if (!(display = init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT, SCL_PIN, SDA_PIN)))
        abort();

    mutex = xSemaphoreCreateMutex();

    xTaskCreate(logo_animation_task, "Logo_Animation", 1024, display, 3, NULL);

    clear_display_buffer(display, 30);
    display_println(display, "Connecting");
    display_println(display, "to wifi");

    xSemaphoreTake(mutex, portMAX_DELAY);
    refresh_display(display);
    xSemaphoreGive(mutex);

    clear_display_buffer(display, 30);
    if (wifi_init_sta() == FAILURE) {
        display_println(display, "Failed to connect to wifi");
        refresh_display(display);
    }
    else {
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));

        display_println(display, "Connected to:");
        display_println(display, (const char *)ap_info.ssid);
        refresh_display(display);
    }


    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));

    size_t required_size;
    error = nvs_get_blob(handle, ACCESS_TOKEN_NVS_KEY, NULL, &required_size);


    if (!(access_token = calloc(1, sizeof(*access_token)))) {
        ESP_LOGE("...", "Failed to allocate memory");
        abort();
    }

    error = nvs_get_blob(handle, ACCESS_TOKEN_NVS_KEY, access_token, &required_size);
    switch (error) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            free(access_token);

            clear_display_buffer(display, 30);
            display_println(display, "Getting");
            display_println(display, "access token");
            refresh_display(display);
            access_token = get_access_token();
            ESP_ERROR_CHECK(nvs_set_blob(handle, ACCESS_TOKEN_NVS_KEY, access_token, sizeof(*access_token)));
            nvs_commit(handle);
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(error));
    }
    nvs_close(handle);


    clear_display_buffer(display, 30);
    display_println(display, "token:");
    display_println(display, access_token->token);
    refresh_display(display);

    printf("token: %s\ncreated_at: %d\nexpires_in: %d\n",
           access_token->token, access_token->created_at, access_token->expires_at);

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    //free(access_token->token);
    free(access_token);
    free(display);
}
