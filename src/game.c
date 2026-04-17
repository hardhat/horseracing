
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

static const char *const horse_color_name[HORSE_COLOR_COUNT] = {
    "WHITE",
    "GREEN",
    "GOLD",
    "RED",
    "BLUE",
    "PURPLE",
    "BROWN",
    "BLACK",
};

static const char *const surface_name[SURFACE_COUNT] = {
    "DRY",
    "MUD",
    "TURF",
};

static const char *const bet_type_name[] = {
    "NONE",
    "WIN",
    "EXA",
};

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
    switch(game.session.selected_field) {
        case BETTING_FIELD_AMOUNT:
            return 4u;
        case BETTING_FIELD_TYPE:
            return 6u;
        //case BETTING_FIELD_HORSE:
        default:
            return (uint8_t)(7u + game.session.selected_entry);
    }
}

static void clear_confirmed_wager(struct Bettor *bettor, struct Wager *wager)
{
    if(wager->confirmed) {
        bettor->bankroll = (uint16_t)(bettor->bankroll + wager->amount);
        wager->confirmed = 0;
    }
}

static void sync_selected_entry(void)
{
    game.session.selected_entry = game.session.wagers[game.session.active_bettor].first_pick;
}

static void normalize_wager(struct Wager *wager)
{
    if(wager->type == BET_TYPE_NONE) {
        wager->type = BET_TYPE_WIN;
    }

    if(wager->amount < 10u) {
        wager->amount = 10u;
    }

    if(wager->first_pick > 3u) {
        wager->first_pick = 0;
    }

    if(wager->type == BET_TYPE_EXACTA) {
        if(wager->second_pick > 3u) {
            wager->second_pick = 1u;
        }
        if(wager->second_pick == wager->first_pick) {
            wager->second_pick = (uint8_t)((wager->first_pick + 1u) & 0x03u);
        }
    } else {
        wager->second_pick = 0;
    }
}

static void adjust_wager_amount(struct Bettor *bettor, struct Wager *wager, int8_t delta)
{
    uint16_t max_amount = bettor->bankroll;

    if(max_amount > 250u) {
        max_amount = 250u;
    }
    if(max_amount < 10u) {
        max_amount = 10u;
    }

    if(delta > 0) {
        if(wager->amount + 10u > max_amount) {
            wager->amount = max_amount;
        } else {
            wager->amount = (uint16_t)(wager->amount + 10u);
        }
    } else if(delta < 0) {
        if(wager->amount <= 10u) {
            wager->amount = 10u;
        } else {
            wager->amount = (uint16_t)(wager->amount - 10u);
        }
    }
}

static void cycle_wager_type(struct Wager *wager, int8_t delta)
{
    if(delta > 0) {
        wager->type = (wager->type == BET_TYPE_EXACTA) ? BET_TYPE_WIN : (uint8_t)(wager->type + 1u);
    } else if(delta < 0) {
        wager->type = (wager->type <= BET_TYPE_WIN) ? BET_TYPE_EXACTA : (uint8_t)(wager->type - 1u);
    }
    normalize_wager(wager);
}

static void cycle_selected_horse(struct Wager *wager, int8_t delta)
{
    if(delta > 0) {
        wager->first_pick = (uint8_t)((wager->first_pick + 1u) & 0x03u);
    } else if(delta < 0) {
        wager->first_pick = (uint8_t)((wager->first_pick + 3u) & 0x03u);
    }
    normalize_wager(wager);
    sync_selected_entry();
}

static void commit_wager(struct Bettor *bettor, struct Wager *wager)
{
    uint16_t max_amount = bettor->bankroll;

    if(max_amount > 250u) {
        max_amount = 250u;
    }
    if(max_amount == 0u) {
        return;
    }
    if(wager->amount > max_amount) {
        wager->amount = max_amount;
    }

    bettor->bankroll = (uint16_t)(bettor->bankroll - wager->amount);
    wager->confirmed = 1;
}

static uint8_t horse_icon_tile(uint8_t color)
{
    switch(color) {
        case HORSE_COLOR_WHITE:
            return TILE_HUD_HORSE_WHITE;
        case HORSE_COLOR_GREEN:
            return TILE_HUD_HORSE_GREEN;
        case HORSE_COLOR_GOLD:
            return TILE_HUD_HORSE_GOLD;
        //case HORSE_COLOR_RED:
        default:
            return TILE_HUD_HORSE_RED;
    }
}

static uint8_t entry_marker_tile(uint8_t entry_index)
{
    switch(entry_index) {
        case 0:
            return TILE_HUD_MARKER_FIRST;
        case 1:
            return TILE_HUD_MARKER_SECOND;
        case 2:
            return TILE_HUD_PP_TIME;
        //case 3:
        default:
            return TILE_HUD_PP_FINISH;
    }
}

static void init_test_session(void)
{
    memset(&game.session, 0, sizeof(game.session));

    game.session.bettor_count = 1;
    game.session.jockey_count = 0;
    game.session.current_race = 0;
    game.session.active_bettor = 0;
    game.session.selected_entry = 0;
    game.session.selected_field = BETTING_FIELD_HORSE;

    game.session.bettors[0].bankroll = 750;
    game.session.bettors[0].active = 1;

    for(uint8_t i = 0; i < 8; i++) {
        game.session.stable[i].color = i;
        game.session.stable[i].traits.break_speed = (uint8_t)(5u + i);
        game.session.stable[i].traits.early_pace = (uint8_t)(6u + i);
        game.session.stable[i].traits.endurance = (uint8_t)(12u - i);
        game.session.stable[i].traits.stretch_speed = (uint8_t)(7u + (i & 0x03u));
        game.session.stable[i].traits.surface_affinity[SURFACE_DRY] = (uint8_t)(8u + (i & 0x01u));
        game.session.stable[i].traits.surface_affinity[SURFACE_MUDDY] = (uint8_t)(6u + ((i + 1u) & 0x01u));
        game.session.stable[i].traits.surface_affinity[SURFACE_TURF] = (uint8_t)(7u + ((i + 2u) & 0x01u));
    }

    game.session.wagers[0].amount = 50;
    game.session.wagers[0].bettor_index = 0;
    game.session.wagers[0].type = BET_TYPE_WIN;
    game.session.wagers[0].first_pick = 0;
    game.session.wagers[0].second_pick = 1;
    game.session.wagers[0].confirmed = 0;

    game.session.races[0].surface = SURFACE_DRY;
    game.session.races[0].distance_furlongs = 6;
    game.session.races[0].entries[0].horse_index = HORSE_COLOR_WHITE;
    game.session.races[0].entries[0].odds = 3;
    game.session.races[0].entries[1].horse_index = HORSE_COLOR_GREEN;
    game.session.races[0].entries[1].odds = 4;
    game.session.races[0].entries[2].horse_index = HORSE_COLOR_GOLD;
    game.session.races[0].entries[2].odds = 5;
    game.session.races[0].entries[3].horse_index = HORSE_COLOR_RED;
    game.session.races[0].entries[3].odds = 6;
}

static void draw_template_screen(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];
    char summary_line[32];
    char wager_line[24];
    char entry_line[16];
    char amount_text[8];
    const char *first_pick_name;

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

    sprintf(summary_line, "R%u %s %uF  $%u",
        (unsigned)(game.session.current_race + 1u),
        surface_name[race->surface],
        (unsigned)race->distance_furlongs,
        (unsigned)bettor->bankroll);

    sprintf(amount_text, "$%03u", (unsigned)wager->amount);
    first_pick_name = horse_color_name[game.session.stable[race->entries[wager->first_pick].horse_index].color];

    if(wager->type == BET_TYPE_EXACTA) {
        const char *second_pick_name = horse_color_name[game.session.stable[race->entries[wager->second_pick].horse_index].color];
        sprintf(wager_line, "%s %s-%s", bet_type_name[wager->type], first_pick_name, second_pick_name);
    } else {
        sprintf(wager_line, "%s %s %s", bet_type_name[wager->type], first_pick_name, amount_text);
    }

    set_font(FONT_NICETY_BOLD);
    draw_text_pair(3, 3,
        "BET SHEET",
        summary_line,
        COL_WHITE, COL_BLACK);
    set_font(FONT_NICETY);
    draw_text_pair(4, 6,
        wager_line,
        wager->confirmed ? "LOCKED   ODDS" : "HORSE    ODDS",
        COL_LIGHT_GRAY, COL_BLACK);

    for(uint8_t i = 0; i < 4; i++) {
        struct RaceEntry *entry = &race->entries[i];
        struct Horse *horse = &game.session.stable[entry->horse_index];

        sprintf(entry_line, "%-6s %u-1", horse_color_name[horse->color], (unsigned)entry->odds);
        draw_bet_entry(horse_icon_tile(horse->color), entry_marker_tile(i), (uint8_t)(7u + i), entry_line);
    }

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

    init_test_session();

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
            if(game.session.selected_field > 0) {
                game.session.selected_field--;
            }
            break;

        case INPUT_DOWN:
            if(game.session.selected_field + 1u < BETTING_FIELD_COUNT) {
                game.session.selected_field++;
            }
            break;

        case INPUT_LEFT:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_AMOUNT:
                    adjust_wager_amount(bettor, wager, -1);
                    break;
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, -1);
                    break;
                case BETTING_FIELD_HORSE:
                    cycle_selected_horse(wager, -1);
                    break;
            }
            resume_game();
            break;
        }

        case INPUT_RIGHT:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_AMOUNT:
                    adjust_wager_amount(bettor, wager, 1);
                    break;
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, 1);
                    break;
                case BETTING_FIELD_HORSE:
                    cycle_selected_horse(wager, 1);
                    break;
            }
            resume_game();
            break;
        }

        case INPUT_SELECT:
            // TODO: Use SELECT for context-sensitive actions in betting or setup screens.
            game_show_dialog("TODO: bind SELECT to horse racing UI action");
            break;

        case INPUT_A:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            if(!wager->confirmed) {
                commit_wager(bettor, wager);
            }
            resume_game();
            break;
        }

        case INPUT_B:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            wager->type = BET_TYPE_WIN;
            wager->amount = 50;
            wager->first_pick = 0;
            wager->second_pick = 1;
            sync_selected_entry();
            resume_game();
            break;
        }

        case INPUT_X:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            cycle_wager_type(wager, 1);
            resume_game();
            break;
        }

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
