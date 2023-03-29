#include <stdio.h>
#include <string.h>
#include <ssd1306/ssd1306.h>
#include <freertos/task.h>
#include "display.h"

static uint8_t frame_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];

void app_main(void)
{
    ssd1306_t   *dev;

    if (!(dev = init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT, SCL_PIN, SDA_PIN)))
        abort();

    clear_display(dev, frame_buffer);
    ssd1306_draw_string(dev, frame_buffer, font_builtin_fonts[0], 0, 0,
                        "Alekol Pocket", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
    ssd1306_load_frame_buffer(dev, frame_buffer);

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    free(dev);
}
