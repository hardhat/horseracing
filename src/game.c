
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "game.h"

#define TEMPLATE_TEXT_TILE ((uint8_t)0xA0u)
#define TEMPLATE_NOTE_TILE ((uint8_t)0xB0u)

struct Game game;

static bool held[MAX_INPUT];

static uint8_t text_tile_span(const char *text)
{
    size_t len = strlen(text);
    return (uint8_t)((len + 1u) >> 1);
}

static void draw_text_strip(uint8_t base_tile, uint8_t map_x, uint8_t map_y,
    const char *text, uint8_t fg, uint8_t bg)
{
    uint8_t span = text_tile_span(text);

    clear_text_tiles(bg, span);
    draw_text(0, 4, text, fg);
    render_text(base_tile, span);

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(base_tile + i));
    }
}

static void draw_template_screen(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);

    // Sample layer usage only. Replace these strips with betting UI and race HUD tiles.
    for(uint8_t x = 0; x < TILES_ACROSS; x++) {
        draw_tilemap(x, 0, TILE_COLOR_TITLE + (x % 17u));
        draw_tilemap(x, 14, TILE_COLOR_HUD + (x % 8u));
    }

    set_font(FONT_NICETY_BOLD);
    draw_text_strip(TEMPLATE_TEXT_TILE, 2, 2, "HORSE RACING", COL_WHITE, COL_BLACK);
    set_font(FONT_NICETY);
    draw_text_strip(TEMPLATE_NOTE_TILE, 1, 5, "TODO: betting sheet", COL_WHITE, COL_BLACK);
    draw_text_strip((uint8_t)(TEMPLATE_NOTE_TILE + 10u), 1, 7, "TODO: past performances", COL_WHITE, COL_BLACK);
    draw_text_strip((uint8_t)(TEMPLATE_NOTE_TILE + 22u), 1, 9, "TODO: live race scene", COL_WHITE, COL_BLACK);
    draw_text_strip((uint8_t)(TEMPLATE_NOTE_TILE + 32u), 1, 12, "D-pad moves sample cursor", COL_LIGHT_GRAY, COL_BLACK);

    render_tilemap(0);
    render_tilemap(1);
}

void game_show_dialog(const char *text)
{
    // Sample hook only. Replace this with a proper modal, status banner, or message box.
    debug_logf("game: %s", text);
}

void resume_game(void)
{
    draw_template_screen();
}

void init_game(void)
{
    debug_log("game: init placeholder screen");

    memset(held, 0, sizeof(held));
    memset(&game, 0, sizeof(game));

    game.cursor_x = 10;
    game.cursor_y = 8;

    clear_sprites();
    resume_game();
}

void input_game(uint8_t key, bool down)
{
    if(key >= MAX_INPUT) {
        return;
    }

    held[key] = down;
    if(!down) {
        return;
    }

    switch(key) {
        case INPUT_START:
            // TODO: Replace with pause menu or end-of-race state handling.
            set_state(GAME_STATE_GAMEOVER);
            break;

        case INPUT_UP:
            if(game.cursor_y > 0) game.cursor_y--;
            break;

        case INPUT_DOWN:
            if(game.cursor_y + 1u < TILES_DOWN) game.cursor_y++;
            break;

        case INPUT_LEFT:
            if(game.cursor_x > 0) game.cursor_x--;
            break;

        case INPUT_RIGHT:
            if(game.cursor_x + 1u < TILES_ACROSS) game.cursor_x++;
            break;

        case INPUT_SELECT:
            // TODO: Use SELECT for context-sensitive actions in betting or setup screens.
            game_show_dialog("TODO: bind SELECT to horse racing UI action");
            break;

        case INPUT_A:
            // TODO: Use A to confirm bets, choose menu items, or trigger race flow.
            game_show_dialog("TODO: bind A to confirm current action");
            break;

        case INPUT_B:
            // TODO: Use B to cancel or back out of menus.
            game_show_dialog("TODO: bind B to cancel current action");
            break;

        default:
            break;
    }
}

void update_game(void)
{
    // TODO: Advance betting flow, race simulation, AI, and animation timers here.
    game.frame_counter++;
}

void draw_game(void)
{
    // Sample sprite usage only. The cursor is a stand-in for a future horse, selector, or UI marker.
    reset_sprite();
    add_sprite((uint16_t)game.cursor_x * 16u, (uint8_t)(game.cursor_y * 16u), TILE_CURSOR);
    render_sprites();
}

void init_gameover(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);

    for(uint8_t y = 0; y < TILES_DOWN; y++) {
        for(uint8_t x = 0; x < TILES_ACROSS; x++) {
            draw_tilemap(x, y, (uint8_t)(TILE_COLOR_WHITE + ((x + y) & 0x03u)));
        }
    }

    set_font(FONT_NICETY_BOLD);
    draw_text_strip(0x90u, 6, 5, "TODO", COL_RED, COL_DARK_RED);
    set_font(FONT_NICETY);
    draw_text_strip(0x94u, 2, 8, "replace with game-over/results", COL_WHITE, COL_BLACK);

    render_tilemap(0);
    clear_sprites();
    debug_log("game: placeholder game-over screen");
}

void input_gameover(uint8_t key, bool down)
{
    if(down && key == INPUT_START) {
        set_state(GAME_STATE_MENU);
    }
}

void update_gameover(void)
{
}

void draw_gameover(void)
{
}
