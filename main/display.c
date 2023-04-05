#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>
#include "display.h"

static const char* TAG = "SSD1306";

void    set_cursor(t_display *display, const uint8_t x, const uint8_t y)
{
    display->cursor.x = x;
    display->cursor.y = y;
}

void    refresh_display(t_display *display)
{
    ssd1306_load_frame_buffer(&display->dev, display->buffer);
}

void        clear_display_buffer(t_display *display, uint8_t from_x)
{
    ssd1306_fill_rectangle(&display->dev, display->buffer, from_x, 0, DISPLAY_WIDTH - from_x,
                           DISPLAY_HEIGHT, OLED_COLOR_BLACK);
    set_cursor(display, from_x + 5, 0);
}

void        clear_display(t_display *display, uint8_t from_x)
{
    clear_display_buffer(display, from_x);
}

void    display_print(t_display *display, const char *str)
{
    ssd1306_draw_string(&display->dev, display->buffer, display->font,
                        display->cursor.x, display->cursor.y,
                        str, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
    display->cursor.x += font_measure_string(display->font, str);
}

void    display_println(t_display *display, const char *str)
{
    ssd1306_draw_string(&display->dev, display->buffer, display->font,
                        display->cursor.x, display->cursor.y,
                        str, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
    display->cursor.x = 35;
    display->cursor.y += display->font->height + 3;
}

void drawBitmap(t_display *display, int8_t x, int8_t y, const uint8_t bitmap[],
                              int8_t w, int8_t h, ssd1306_color_t color) {

  int8_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t b = 0;

  for (int8_t j = 0; j < h; j++, y++) {
    for (int8_t i = 0; i < w; i++) {
      if (i & 7)
        b <<= 1;
      else
        b = bitmap[j * byteWidth + i / 8];
      if (b & 0x80)
        ssd1306_draw_pixel(&display->dev, display->buffer, x + i, y, color);
    }
  }
}


t_display	*init_display(uint8_t width, uint8_t height,
                        uint8_t scl_pin, uint8_t sda_pin)
{
    i2c_config_t	conf;
    int			    i2c_master_port;
    t_display	    *display;

    if (!(display = malloc(sizeof(*display)))) {
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

    display->dev.i2c_port = i2c_master_port;
    display->dev.i2c_addr = SSD1306_I2C_ADDR_0;
    display->dev.screen = SSD1306_SCREEN;
    display->dev.width = width;
    display->dev.height = height;

    if (ssd1306_init(&display->dev) != 0) {
        free(display);
        ESP_LOGE(TAG, "%s: ssd1306_init() failed", __func__);
        return (NULL);
    }
    set_cursor(display, 0, 0);
    display->font = font_builtin_fonts[0];
    ssd1306_set_whole_display_lighting(&display->dev, false);
    clear_display_buffer(display, 0);
    ESP_LOGI(TAG, "Display successfully init");
    return (display);
}
