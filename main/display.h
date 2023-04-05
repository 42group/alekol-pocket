#ifndef DISPLAY_H
#define DISPLAY_H

#include <ssd1306/ssd1306.h>

#define SCL_PIN 5
#define SDA_PIN 4
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

typedef struct s_cursor
{
    uint8_t x;
    uint8_t y;
}              t_cursor;

typedef struct s_display
{
    ssd1306_t           dev;
    const font_info_t   *font;
    t_cursor            cursor;
    uint8_t             buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];
}               t_display;

t_display	*init_display(uint8_t width, uint8_t height,
                        uint8_t scl_pin, uint8_t sda_pin);
void        clear_display(t_display *display, uint8_t from_x);
void        clear_display_buffer(t_display *display, uint8_t from_x);
void    refresh_display(t_display *display);
void    display_println(t_display *display, const char *str);
void    display_print(t_display *display, const char *str);
void    set_cursor(t_display *display, const uint8_t x, const uint8_t y);
void drawBitmap(t_display *display, int8_t x, int8_t y, const uint8_t bitmap[],
                              int8_t w, int8_t h, ssd1306_color_t color);

#endif
