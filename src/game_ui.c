#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "main.h"
#include "text.h"
#include "game_ui.h"

static void copy_padded_text(char *buffer, uint8_t width, const char *value)
{
    uint8_t length = (uint8_t)strlen(value);

    if(length > width) {
        length = width;
    }

    memset(buffer, ' ', width);
    memcpy(buffer, value, length);
    buffer[width] = '\0';
}

uint8_t game_ui_centered_strip_x(const char *text)
{
    uint8_t span = text_measure_span(text);

    if(span >= TILES_ACROSS) {
        return 0u;
    }

    return (uint8_t)((TILES_ACROSS - span) >> 1u);
}

uint8_t game_ui_horse_text_color(uint8_t color)
{
    switch(color) {
        case HORSE_COLOR_GREEN:
            return COL_GREEN;

        case HORSE_COLOR_GOLD:
            return COL_YELLOW;

        case HORSE_COLOR_RED:
            return COL_RED;

        case HORSE_COLOR_BLUE:
            return COL_BLUE;

        case HORSE_COLOR_PURPLE:
            return COL_MAGENTA;

        case HORSE_COLOR_BROWN:
            return COL_BROWN;

        case HORSE_COLOR_BLACK:
            return COL_LIGHT_GRAY;

        default:
            return COL_WHITE;
    }
}

static void draw_board_shell(void)
{
    for(uint8_t y = 0u; y < TILES_DOWN; y++) {
        for(uint8_t x = 0u; x < TILES_ACROSS; x++) {
            uint8_t tile;

            if(y == 0u) {
                if(x == 0u) {
                    tile = TILE_THEME_TOP_LEFT;
                } else if(x == (uint8_t)(TILES_ACROSS - 1u)) {
                    tile = TILE_THEME_TOP_RIGHT;
                } else {
                    tile = ((x & 1u) == 0u) ? TILE_THEME_TOP_FILL_A : TILE_THEME_TOP_FILL_B;
                }
            } else if(y == (uint8_t)(TILES_DOWN - 1u)) {
                if(x == 0u) {
                    tile = TILE_THEME_BOTTOM_LEFT;
                } else if(x == (uint8_t)(TILES_ACROSS - 1u)) {
                    tile = TILE_THEME_BOTTOM_RIGHT;
                } else {
                    tile = ((x & 1u) == 0u) ? TILE_THEME_BOTTOM_FILL_A : TILE_THEME_BOTTOM_FILL_B;
                }
            } else if(x == 0u) {
                tile = TILE_THEME_MID_LEFT;
            } else if(x == (uint8_t)(TILES_ACROSS - 1u)) {
                tile = TILE_THEME_MID_RIGHT;
            } else {
                tile = (((x + y) & 1u) == 0u) ? TILE_THEME_MID_FILL_A : TILE_THEME_MID_FILL_B;
            }

            draw_tilemap(x, y, tile);
        }
    }

    for(uint8_t y = 1u; y < (uint8_t)(TILES_DOWN - 1u); y++) {
        for(uint8_t x = 1u; x < (uint8_t)(TILES_ACROSS - 1u); x++) {
            uint8_t body_tile = (((x >> 1u) + y) & 1u) == 0u
                ? TILE_THEME_BODY_A
                : TILE_THEME_BODY_B;
            draw_tilemap(x, y, body_tile);
        }
    }

    draw_tilemap(2, 2, TILE_THEME_ACCENT);
    draw_tilemap(17, 2, TILE_THEME_ACCENT);
    draw_tilemap(2, 12, TILE_THEME_ACCENT);
    draw_tilemap(17, 12, TILE_THEME_ACCENT);
}

void game_ui_clear_overlay_layer(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
}

void game_ui_draw_board_background_layer(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    draw_board_shell();
    render_tilemap(0);
}

void game_ui_format_bettor_status(char *buffer, uint8_t bettor_index)
{
    if(bettor_index < game.session.bettor_count
        && game.session.bettors[bettor_index].bankroll == 0u) {
        sprintf(buffer, "B%u OUT", (unsigned)(bettor_index + 1u));
        return;
    }

    sprintf(buffer, "B%u $%u",
        (unsigned)(bettor_index + 1u),
        (unsigned)game.session.bettors[bettor_index].bankroll);
}

void game_ui_draw_field_value(uint16_t x, uint8_t y, uint8_t width,
    const char *value, bool selected)
{
    char padded[10];

    copy_padded_text(padded, width, value);
    draw_text_opaque(x, y, padded,
        COL_WHITE,
        selected ? COL_DARK_BLUE : COL_BLACK);
}