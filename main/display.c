#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ssd1306/ssd1306.h>
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>
#include "display.h"

static const char* TAG = "SSD1306";

void        clear_display(ssd1306_t *dev, uint8_t *frame_buffer)
{
    ssd1306_fill_rectangle(dev, frame_buffer, 0, 0, DISPLAY_WIDTH,
                           DISPLAY_HEIGHT, OLED_COLOR_BLACK);
}

ssd1306_t	*init_display(uint8_t width, uint8_t height,
                        uint8_t scl_pin, uint8_t sda_pin)
{
    i2c_config_t	conf;
    int			    i2c_master_port;
    ssd1306_t	    *dev;

    if (!(dev = malloc(sizeof(*dev)))) {
        ESP_LOGE(TAG, "%s: failed to allocate memory", __func__);
        return (NULL);
    }

    i2c_master_port = I2C_NUM_0;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = scl_pin;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 300;
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));

    dev->i2c_port = i2c_master_port;
    dev->i2c_addr = SSD1306_I2C_ADDR_0;
    dev->screen = SSD1306_SCREEN;
    dev->width = width;
    dev->height = height;

    if (ssd1306_init(dev) != 0) {
        free(dev);
        ESP_LOGE(TAG, "%s: ssd1306_init() failed", __func__);
        return (NULL);
    }
    ssd1306_set_whole_display_lighting(dev, false);
    ESP_LOGI(TAG, "Display successfully init");
    return (dev);
}
