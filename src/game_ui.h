#ifndef GAME_UI_H
#define GAME_UI_H

#include <stdbool.h>
#include <stdint.h>

uint8_t game_ui_centered_strip_x(const char *text);
uint8_t game_ui_horse_text_color(uint8_t color);
void game_ui_draw_board_background_layer(void);
void game_ui_clear_overlay_layer(void);
void game_ui_format_bettor_status(char *buffer, uint8_t bettor_index);
void game_ui_draw_field_value(uint16_t x, uint8_t y, uint8_t width,
    const char *value, bool selected);

#endif