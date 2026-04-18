#include <stdio.h>
#include <string.h>

#include "main.h"
#include "menu.h"
#include "game.h"
#include "dat.h"

#define TITLE_PROMPT_TILE 0xD0
#define SETUP_HEADER_TILE 0xD8
#define SETUP_VALUE_TILE  0xE0
#define HELP_TILE         0xE8
#define ARROW_TILE        0xAF

#define MAX_BLINK_TIMER 8

enum MenuScreen {
    MENU_SCREEN_TITLE,
    MENU_SCREEN_BETTORS,
};

static uint8_t menu_screen;
static uint8_t selected_bettors;
static int blink_timer = MAX_BLINK_TIMER;
static uint8_t prompt_bg = COL_BLUE;

static void draw_title_banner(void)
{
    uint8_t buffer[17 + 17];

    for(uint8_t i = 0; i < 34u; i++) {
        buffer[i] = (uint8_t)(TILE_COLOR_TITLE + i);
    }

    for(uint8_t row = 0; row < 2u; row++) {
        draw_tilemap_array(2, (uint8_t)(2u + row), &buffer[row * 17u], 17);
    }
}

static void draw_center_strip(uint8_t tile, uint8_t map_x, uint8_t map_y,
    uint8_t bg, uint8_t fg, const char *text)
{
    uint8_t span = (uint8_t)((strlen(text) + 1u) >> 1);

    clear_text_tiles(bg, span);
    draw_text_opaque(0, 4, text, fg, bg);
    render_text(tile, span);

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(tile + i));
    }
}

static void redraw_menu(void)
{
    char bettor_line[16];

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);
    clear_sprites();

    draw_title_banner();

    set_font(FONT_NICETY_BOLD);
    draw_center_strip(TITLE_PROMPT_TILE, 4, 6, COL_BLACK, COL_WHITE, "HORSE RACING");

    set_font(FONT_NICETY);
    if(menu_screen == MENU_SCREEN_TITLE) {
        draw_center_strip(SETUP_HEADER_TILE, 3, 9, prompt_bg, COL_WHITE, "PRESS START");
        draw_center_strip(HELP_TILE, 2, 12, COL_BLACK, COL_LIGHT_GRAY, "NEW 10-RACE PROGRAM");
    } else {
        draw_center_strip(SETUP_HEADER_TILE, 4, 8, COL_BLACK, COL_LIGHT_GRAY, "SELECT BETTORS");
        sprintf(bettor_line, "<  %u BETTORS  >", (unsigned)selected_bettors);
        draw_center_strip(SETUP_VALUE_TILE, 3, 10, COL_DARK_BLUE, COL_WHITE, bettor_line);
        draw_center_strip(HELP_TILE, 1, 13, COL_BLACK, COL_LIGHT_GRAY, "U/D CHANGE  A START");
    }

    render_tilemap(0);
}

void init_menu(void)
{
    menu_screen = MENU_SCREEN_TITLE;
    selected_bettors = (game.configured_bettors >= 1u && game.configured_bettors <= 6u)
        ? game.configured_bettors
        : 1u;
    blink_timer = MAX_BLINK_TIMER;
    prompt_bg = COL_BLUE;
    redraw_menu();
}

void input_menu(uint8_t key, bool down)
{
    if(!down) {
        return;
    }

    if(menu_screen == MENU_SCREEN_TITLE) {
        if(key == INPUT_START || key == INPUT_A) {
            menu_screen = MENU_SCREEN_BETTORS;
            redraw_menu();
        }
        return;
    }

    switch(key) {
        case INPUT_UP:
        case INPUT_RIGHT:
            if(selected_bettors < 6u) {
                selected_bettors++;
                redraw_menu();
            }
            break;

        case INPUT_DOWN:
        case INPUT_LEFT:
            if(selected_bettors > 1u) {
                selected_bettors--;
                redraw_menu();
            }
            break;

        case INPUT_B:
            menu_screen = MENU_SCREEN_TITLE;
            redraw_menu();
            break;

        case INPUT_A:
        case INPUT_START:
            game.configured_bettors = selected_bettors;
            set_state(GAME_STATE_GAME);
            break;

        default:
            break;
    }
}

void update_menu(void)
{
    blink_timer--;
    if(blink_timer == 0) {
        blink_timer = MAX_BLINK_TIMER;
        prompt_bg = (prompt_bg == COL_BLUE) ? COL_DARK_BLUE : COL_BLUE;
        if(menu_screen == MENU_SCREEN_TITLE) {
            redraw_menu();
        }
    }
}

void draw_menu(void)
{
    reset_sprite();
    if(menu_screen == MENU_SCREEN_BETTORS) {
        add_sprite(2u * 16u, (uint8_t)(10u * 16u + 4u), ARROW_TILE);
        add_sprite(16u * 16u, (uint8_t)(10u * 16u + 4u), ARROW_TILE);
    }
    render_sprites();
}