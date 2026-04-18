#include <stdio.h>
#include <string.h>

#include "main.h"
#include "menu.h"
#include "game.h"
#include "dat.h"
#include "text.h"

#define ARROW_TILE        0xAF
#define MENU_TEXT_BASE    ((uint8_t)0xD0u)
#define MENU_TEXT_LIMIT   ((uint8_t)0xFFu)

#define MAX_BLINK_TIMER 8

enum MenuScreen {
    MENU_SCREEN_TITLE,
    MENU_SCREEN_BETTORS,
};

static uint8_t menu_screen;
static uint8_t selected_bettors;
static int blink_timer = MAX_BLINK_TIMER;
static uint8_t prompt_bg = COL_BLUE;

static struct TextStripBlock menu_title_block;
static struct TextStripBlock title_prompt_block;
static struct TextStripBlock title_help_block;
static struct TextStripBlock setup_header_block;
static struct TextStripBlock setup_value_block;
static struct TextStripBlock setup_help_block;

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

static void init_menu_arrow_tile(void)
{
    clear_text_tiles(COL_BLACK, 1);
    draw_text(0, 4, "<", COL_WHITE);
    render_text(ARROW_TILE, 1);
}

static void update_title_screen_blocks(void)
{
    title_prompt_block.bg = prompt_bg;
    text_update_strip_block(&menu_title_block, "HORSE RACING");
    text_update_strip_block(&title_prompt_block, "PRESS START");
    text_update_strip_block(&title_help_block, "NEW 10-RACE PROGRAM");
    text_update_strip_block(&setup_header_block, "");
    text_update_strip_block(&setup_value_block, "");
    text_update_strip_block(&setup_help_block, "");
}

static void update_bettor_setup_blocks(void)
{
    char bettor_line[16];

    text_update_strip_block(&menu_title_block, "HORSE RACING");
    text_update_strip_block(&title_prompt_block, "");
    text_update_strip_block(&title_help_block, "");
    text_update_strip_block(&setup_header_block, "SELECT BETTORS");
    sprintf(bettor_line, "<  %u BETTORS  >", (unsigned)selected_bettors);
    text_update_strip_block(&setup_value_block, bettor_line);
    text_update_strip_block(&setup_help_block, "U/D CHANGE  A START");
}

static void build_menu_layout(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);
    clear_sprites();

    draw_title_banner();
    init_menu_arrow_tile();
    text_reset_arena(MENU_TEXT_BASE, MENU_TEXT_LIMIT);

    set_font(FONT_NICETY_BOLD);
    text_init_strip_block(&menu_title_block, 4, 6,
        text_measure_span("HORSE RACING"), COL_WHITE, COL_BLACK, 4, true);

    set_font(FONT_NICETY);
    text_init_strip_block(&title_prompt_block, 3, 9,
        text_measure_span("PRESS START"), COL_WHITE, prompt_bg, 4, true);
    text_init_strip_block(&title_help_block, 2, 12,
        text_measure_span("NEW 10-RACE PROGRAM"), COL_LIGHT_GRAY, COL_BLACK, 4, true);
    text_init_strip_block(&setup_header_block, 4, 8,
        text_measure_span("SELECT BETTORS"), COL_LIGHT_GRAY, COL_BLACK, 4, true);
    text_init_strip_block(&setup_value_block, 3, 10,
        text_measure_span("<  6 BETTORS  >"), COL_WHITE, COL_DARK_BLUE, 4, true);
    text_init_strip_block(&setup_help_block, 1, 13,
        text_measure_span("U/D CHANGE  A START"), COL_LIGHT_GRAY, COL_BLACK, 4, true);

    update_title_screen_blocks();
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
    build_menu_layout();
}

void input_menu(uint8_t key, bool down)
{
    if(!down) {
        return;
    }

    if(menu_screen == MENU_SCREEN_TITLE) {
        if(key == INPUT_START || key == INPUT_A) {
            menu_screen = MENU_SCREEN_BETTORS;
            update_bettor_setup_blocks();
        }
        return;
    }

    switch(key) {
        case INPUT_UP:
        case INPUT_RIGHT:
            if(selected_bettors < 6u) {
                selected_bettors++;
                update_bettor_setup_blocks();
            }
            break;

        case INPUT_DOWN:
        case INPUT_LEFT:
            if(selected_bettors > 1u) {
                selected_bettors--;
                update_bettor_setup_blocks();
            }
            break;

        case INPUT_B:
            menu_screen = MENU_SCREEN_TITLE;
            update_title_screen_blocks();
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
            update_title_screen_blocks();
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