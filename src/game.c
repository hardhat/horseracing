
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "game.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xD8u)

struct Game game;

static bool held[MAX_INPUT];
static uint8_t text_tile_next;

static uint8_t text_tile_span(const char *text)
{
    size_t len = strlen(text);
    return (uint8_t)((len + 1u) >> 1);
}

static uint8_t max_u8(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

static void reset_text_tile_arena(void)
{
    text_tile_next = TEXT_TILE_ARENA_BASE;
}

static uint8_t alloc_text_tiles(uint8_t span)
{
    uint8_t base = text_tile_next;

    if(span == 0) {
        return base;
    }

    if((uint16_t)base + span > TEXT_TILE_ARENA_LIMIT) {
        debug_log("game: text tile arena exhausted");
        return TEXT_TILE_ARENA_BASE;
    }

    text_tile_next = (uint8_t)(text_tile_next + span);
    return base;
}

static void draw_text_strip(uint8_t map_x, uint8_t map_y,
    const char *text, uint8_t fg, uint8_t bg)
{
    uint8_t span = text_tile_span(text);
    uint8_t base_tile = alloc_text_tiles(span);

    clear_text_tiles(bg, span);
    draw_text(0, 4, text, fg);
    render_text(base_tile, span);

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(base_tile + i));
    }
}

static void draw_text_pair(uint8_t map_x, uint8_t map_y,
    const char *top_text, const char *bottom_text, uint8_t fg, uint8_t bg)
{
    uint8_t span = max_u8(text_tile_span(top_text), text_tile_span(bottom_text));
    uint8_t base_tile = alloc_text_tiles(span);

    clear_text_tiles(bg, span);
    draw_text(0, 0, top_text, fg);
    draw_text(0, 8, bottom_text, fg);
    render_text(base_tile, span);

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(base_tile + i));
    }
}

static void draw_bet_entry(uint8_t icon_tile, uint8_t marker_tile, uint8_t map_y,
    const char *text)
{
    draw_tilemap(1, map_y, icon_tile);
    draw_tilemap(2, map_y, marker_tile);
    draw_text_strip(4, map_y, text, COL_WHITE, COL_BLACK);
}

static uint8_t selected_row_y(void)
{
    return (uint8_t)(7u + (game.cursor_y & 0x03u));
}

static void draw_template_screen(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    reset_text_tile_arena();

    for(uint8_t x = 0; x < 17u; x++) {
        draw_tilemap((uint8_t)(x + 1u), 0, (uint8_t)(TILE_COLOR_TITLE + x));
        draw_tilemap((uint8_t)(x + 1u), 1, (uint8_t)(TILE_COLOR_TITLE + 17u + x));
    }

    static const uint8_t prototype_track_row[] = {
        TILE_TRACK_DRY_GRANDSTAND_0,
        TILE_TRACK_DRY_GRANDSTAND_1,
        TILE_TRACK_DRY_CROWD_0,
        TILE_TRACK_DRY_CROWD_1,
        TILE_TRACK_DRY_CLEAR_0,
        TILE_TRACK_DRY_CLEAR_1,
        TILE_TRACK_DRY_FINISH,
        TILE_TRACK_DRY_FURLONG,
        TILE_TRACK_DRY_HORSE_LIGHT,
        TILE_TRACK_DRY_HORSE_GREEN,
        TILE_TRACK_DRY_MARKER,
        TILE_TRACK_UTIL_RAIL_CURVE_0,
        TILE_TRACK_UTIL_RAIL_CURVE_1,
        TILE_TRACK_UTIL_RAIL_CURVE_2,
        TILE_TRACK_UTIL_RAIL_CURVE_3,
        TILE_TRACK_UTIL_POST_0,
        TILE_TRACK_UTIL_POST_1,
        TILE_TRACK_UTIL_POST_2,
        TILE_TRACK_UTIL_POST_3,
        TILE_TRACK_UTIL_DRY_0,
    };
    draw_tilemap_array(0, 11, (uint8_t *)prototype_track_row, sizeof(prototype_track_row));

    set_font(FONT_NICETY_BOLD);
    draw_text_pair(3, 3,
        "BET SHEET",
        "R1 DRY 6F  $750",
        COL_WHITE, COL_BLACK);
    set_font(FONT_NICETY);
    draw_text_pair(4, 6,
        "WIN WHITE",
        "HORSE    ODDS",
        COL_LIGHT_GRAY, COL_BLACK);

    draw_bet_entry(TILE_HUD_HORSE_WHITE, TILE_HUD_MARKER_FIRST, 7, "WHITE   3-1");
    draw_bet_entry(TILE_HUD_HORSE_GREEN, TILE_HUD_MARKER_SECOND, 8, "GREEN   4-1");
    draw_bet_entry(TILE_HUD_HORSE_GOLD, TILE_HUD_PP_TIME, 9, "GOLD    5-1");
    draw_bet_entry(TILE_HUD_HORSE_RED, TILE_HUD_PP_FINISH, 10, "RED     6-1");

    draw_text_pair(1, 13,
        "A BET  Y FORM",
        "START  B BACK",
        COL_LIGHT_GRAY, COL_BLACK);

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
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);

    draw_template_screen();
}

void init_game(void)
{
    debug_log("game: init placeholder screen");

    memset(held, 0, sizeof(held));
    memset(&game, 0, sizeof(game));

    game.cursor_x = 0;
    game.cursor_y = 0;

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
            if(game.cursor_y < 3u) game.cursor_y++;
            break;

        case INPUT_LEFT:
            break;

        case INPUT_RIGHT:
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
    // Row selector for the current betting entry.
    reset_sprite();
    add_sprite(0, (uint8_t)(selected_row_y() * 16u), TILE_CURSOR);
    render_sprites();
}

void init_gameover(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);

    reset_text_tile_arena();

    for(uint8_t y = 0; y < TILES_DOWN; y++) {
        for(uint8_t x = 0; x < TILES_ACROSS; x++) {
            draw_tilemap(x, y, (uint8_t)(TILE_COLOR_WHITE + ((x + y) & 0x03u)));
        }
    }

    set_font(FONT_NICETY_BOLD);
    draw_text_strip(6, 5, "TODO", COL_RED, COL_DARK_RED);
    set_font(FONT_NICETY);
    draw_text_strip(2, 8, "replace with game-over/results", COL_WHITE, COL_BLACK);

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
