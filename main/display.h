#ifndef DISPLAY_H
#define DISPLAY_H

#define SCL_PIN 5
#define SDA_PIN 4
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

ssd1306_t	*init_display(uint8_t width, uint8_t height,
                        uint8_t scl_pin, uint8_t sda_pin);
void        clear_display(ssd1306_t *dev, uint8_t *frame_buffer);

#endif
