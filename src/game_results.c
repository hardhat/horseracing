#include <stdio.h>

#include "game.h"
#include "game_results.h"
#include "game_ui.h"
#include "main.h"
#include "text.h"
#include "wager_math.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xFFu)

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

static bool bettor_is_busted(uint8_t bettor_index)
{
    return bettor_index < game.session.bettor_count
        && game.session.bettors[bettor_index].bankroll == 0u;
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
        default:
            return TILE_HUD_HORSE_RED;
    }
}

static void build_standings_order(uint8_t standings_order[6])
{
    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        standings_order[i] = i;
    }

    for(uint8_t i = 0; i + 1u < game.session.bettor_count; i++) {
        for(uint8_t j = (uint8_t)(i + 1u); j < game.session.bettor_count; j++) {
            uint8_t left = standings_order[i];
            uint8_t right = standings_order[j];
            uint16_t left_bank = game.session.bettors[left].bankroll;
            uint16_t right_bank = game.session.bettors[right].bankroll;

            if(right_bank > left_bank
                || (right_bank == left_bank && right < left)) {
                uint8_t swap = standings_order[i];
                standings_order[i] = standings_order[j];
                standings_order[j] = swap;
            }
        }
    }
}

void game_results_render_race_results_screen(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    char line[20];
    char status_line[12];

    reset_layer_scrolls();
    game_ui_draw_board_background_layer();
    game_ui_clear_overlay_layer();
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

    text_draw_strip_alloc(game_ui_centered_strip_x("OFFICIAL RESULTS"), 2,
        "OFFICIAL RESULTS", COL_YELLOW, COL_BLACK, 4, true);
    set_font(FONT_NICETY_BOLD);
    sprintf(line, "R%u  WIN %u",
        (unsigned)(game.session.current_race + 1u),
        (unsigned)race->result.winning_time);
    text_draw_strip_alloc(game_ui_centered_strip_x(line), 3, line, COL_WHITE, COL_BLACK, 4, true);
    set_font(FONT_NICETY);
    text_draw_strip_alloc(2, 5, "POS HORSE      ODDS", COL_LIGHT_GRAY, COL_BLACK, 4, true);

    for(uint8_t i = 0; i < 4u; i++) {
        uint8_t entry_index = race->result.finish_order[i];
        struct RaceEntry *entry = &race->entries[entry_index];
        struct Horse *horse = &game.session.stable[race->entries[entry_index].horse_index];
        uint8_t row_y = (uint8_t)(6u + i);

        sprintf(line, "%u", (unsigned)(i + 1u));
        text_draw_strip_alloc(2, row_y, line, i == 0u ? COL_YELLOW : COL_WHITE, COL_BLACK, 4, false);
        draw_tilemap(4, row_y, horse_icon_tile(horse->color));
        text_draw_strip_alloc(5, row_y, horse_color_name[horse->color],
            game_ui_horse_text_color(horse->color), COL_BLACK, 4, false);
        sprintf(line, "%u-1", (unsigned)entry->odds);
        text_draw_strip_alloc(14, row_y, line, COL_YELLOW, COL_BLACK, 4, false);
    }

    text_draw_strip_alloc(2, 10, "BETTORS", COL_LIGHT_GRAY, COL_BLACK, 4, true);
    text_draw_strip_alloc(11, 10, "AUTO NEXT", COL_WHITE, COL_BLACK, 4, true);

    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        uint8_t x = (i & 1u) == 0u ? 2u : 11u;
        uint16_t payout = wager_math_payout(&game.session.wagers[i], race);

        if(payout > 0u) {
            sprintf(status_line, "B%u +$%u", (unsigned)(i + 1u), (unsigned)payout);
        } else if(bettor_is_busted(i)) {
            sprintf(status_line, "B%u OUT", (unsigned)(i + 1u));
        } else if(game.session.wagers[i].type == BET_TYPE_NONE) {
            sprintf(status_line, "B%u PASS", (unsigned)(i + 1u));
        } else {
            sprintf(status_line, "B%u MISS", (unsigned)(i + 1u));
        }

        text_draw_strip_alloc(x, (uint8_t)(11u + (i >> 1u)), status_line,
            payout > 0u ? COL_YELLOW : (bettor_is_busted(i) ? COL_RED : COL_WHITE),
            COL_BLACK, 4, false);
    }

    render_tilemap(1);
}

void game_results_render_final_standings_screen(void)
{
    char line[20];
    char status_line[12];
    uint8_t winner_index;
    uint8_t standings_order[6];

    reset_layer_scrolls();
    game_ui_draw_board_background_layer();
    game_ui_clear_overlay_layer();
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

    build_standings_order(standings_order);
    winner_index = standings_order[0];

    text_draw_strip_alloc(game_ui_centered_strip_x("FINAL STANDINGS"), 2,
        "FINAL STANDINGS", COL_YELLOW, COL_BLACK, 4, true);
    set_font(FONT_NICETY_BOLD);

    game_ui_format_bettor_status(status_line, winner_index);
    sprintf(line, "WINNER %s", status_line);
    text_draw_strip_alloc(game_ui_centered_strip_x(line), 3, line, COL_YELLOW, COL_BLACK, 4, true);
    set_font(FONT_NICETY);
    text_draw_strip_alloc(2, 5, "RK BETTOR     BANK", COL_LIGHT_GRAY, COL_BLACK, 4, true);

    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        uint8_t bettor_index = standings_order[i];
        uint8_t row_y = (uint8_t)(6u + i);

        sprintf(line, "%u", (unsigned)(i + 1u));
        text_draw_strip_alloc(2, row_y, line, i == 0u ? COL_YELLOW : COL_WHITE, COL_BLACK, 4, false);
        sprintf(status_line, "B%u", (unsigned)(bettor_index + 1u));
        text_draw_strip_alloc(4, row_y, status_line,
            bettor_is_busted(bettor_index) ? COL_RED : COL_WHITE,
            COL_BLACK, 4, false);
        if(bettor_is_busted(bettor_index)) {
            text_draw_strip_alloc(11, row_y, "OUT", COL_RED, COL_BLACK, 4, false);
        } else {
            sprintf(status_line, "$%u", (unsigned)game.session.bettors[bettor_index].bankroll);
            text_draw_strip_alloc(11, row_y, status_line,
                i == 0u ? COL_YELLOW : COL_WHITE,
                COL_BLACK, 4, false);
        }
    }

    text_draw_strip_alloc(4, 13, "RACE 10 DONE  START", COL_WHITE, COL_BLACK, 4, true);

    render_tilemap(1);
}