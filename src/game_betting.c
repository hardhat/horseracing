#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "game_betting.h"
#include "game_ui.h"
#include "main.h"
#include "text.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xFFu)
#define BETTING_SUMMARY_SPAN  ((uint8_t)12u)
#define BETTING_WAGER_SPAN    ((uint8_t)12u)

static struct TextPairBlock betting_summary_block;
static struct TextPairBlock betting_wager_block;
static struct TextPairBlock betting_help_block;

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

static const char *const horse_color_short_name[HORSE_COLOR_COUNT] = {
    "WHT",
    "GRN",
    "GLD",
    "RED",
    "BLU",
    "PUR",
    "BRN",
    "BLK",
};

static const char *const surface_name[SURFACE_COUNT] = {
    "DRY",
    "MUD",
    "TURF",
};

static const char surface_code[SURFACE_COUNT] = {
    'D',
    'M',
    'T',
};

static const char *const bet_type_name[] = {
    "NONE",
    "WIN",
    "EXA",
};

static bool bettor_is_busted(uint8_t bettor_index)
{
    return bettor_index < game.session.bettor_count
        && game.session.bettors[bettor_index].bankroll == 0u;
}

static const char *jockey_control_label(void)
{
    return (game.session.jockey_assignment == JOCKEY_ASSIGN_CPU) ? "CPU" : "P1";
}

static const char *jockey_horse_label(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];

    switch(game.session.jockey_assignment) {
        case JOCKEY_ASSIGN_OUTSIDE_1:
            return horse_color_short_name[game.session.stable[race->entries[0].horse_index].color];

        case JOCKEY_ASSIGN_OUTSIDE_4:
            return horse_color_short_name[game.session.stable[race->entries[3].horse_index].color];

        default:
            return "NONE";
    }
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

static uint8_t entry_marker_tile(uint8_t entry_index)
{
    switch(entry_index) {
        case 0:
            return TILE_HUD_MARKER_FIRST;
        case 1:
            return TILE_HUD_MARKER_SECOND;
        case 2:
            return TILE_HUD_PP_TIME;
        default:
            return TILE_HUD_PP_FINISH;
    }
}

static void update_wager_block(const struct TextPairBlock *block,
    const char *type_value, const char *amount_value, const char *horse_value,
    const char *horse_label,
    bool type_selected, bool amount_selected, bool horse_selected,
    const char *state_label)
{
    clear_text_tiles(block->bg, block->span);

    draw_text(0, 0, "TYPE", COL_LIGHT_GRAY);
    game_ui_draw_field_value(40, 0, 4, type_value, type_selected);
    draw_text(80, 0, "AMT", COL_LIGHT_GRAY);
    game_ui_draw_field_value(112, 0, 5, amount_value, amount_selected);

    draw_text(0, 8, horse_label, COL_LIGHT_GRAY);
    game_ui_draw_field_value(48, 8, 7, horse_value, horse_selected);
    draw_text(120, 8, state_label, COL_LIGHT_GRAY);

    render_text(block->base_tile, block->span);
}

static void update_jockey_block(const struct TextPairBlock *block,
    const char *control_value, const char *horse_value, const char *state_label)
{
    clear_text_tiles(block->bg, block->span);

    draw_text(0, 0, "CTRL", COL_LIGHT_GRAY);
    game_ui_draw_field_value(40, 0, 4, control_value, true);
    draw_text(80, 0, "PHASE", COL_LIGHT_GRAY);
    draw_text(120, 0, state_label, COL_LIGHT_GRAY);

    draw_text(0, 8, "HORSE", COL_LIGHT_GRAY);
    game_ui_draw_field_value(48, 8, 7, horse_value, true);

    render_text(block->base_tile, block->span);
}

static void draw_bet_entry(uint8_t icon_tile, uint8_t marker_tile, uint8_t map_y,
    const char *horse_name, uint8_t horse_color, uint8_t odds)
{
    draw_tilemap(1, map_y, icon_tile);
    draw_tilemap(2, map_y, marker_tile);
    text_draw_strip_alloc(4, map_y, horse_name, game_ui_horse_text_color(horse_color), COL_BLACK, 4, false);

    {
        char line[8];
        sprintf(line, "%u-1", (unsigned)odds);
        text_draw_strip_alloc(14, map_y, line, COL_YELLOW, COL_BLACK, 4, false);
    }
}

static void render_past_performance_rows(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct RaceEntry *entry = &race->entries[game.session.past_performance_entry];
    struct Horse *horse = &game.session.stable[entry->horse_index];
    char line[20];

    text_draw_strip_alloc(1, 7, " F S TIME P  TOP2 ", COL_LIGHT_GRAY, COL_BLACK, 4, false);

    if(horse->history_count == 0u) {
        text_draw_strip_alloc(3, 9, "NO PRIOR RUNS", COL_WHITE, COL_BLACK, 4, false);
        return;
    }

    for(uint8_t i = 0; i < horse->history_count; i++) {
        const struct PastPerformance *performance = &horse->history[horse->history_count - 1u - i];
        uint8_t winner_color = (uint8_t)(performance->top2 >> 4);
        uint8_t runner_up_color = (uint8_t)(performance->top2 & 0x0Fu);

        sprintf(line, "%2uF %c %4u %u %s/%s",
            (unsigned)performance->distance_furlongs,
            surface_code[performance->surface],
            (unsigned)performance->finish_time,
            (unsigned)performance->finish_position,
            horse_color_short_name[winner_color],
            horse_color_short_name[runner_up_color]);
        text_draw_strip_alloc(0, (uint8_t)(8u + i), line, COL_WHITE, COL_BLACK, 4, false);
    }
}

static void update_betting_text_blocks(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];
    char title_line[16];
    char summary_line[32];
    char amount_value[8];
    char horse_value[12];
    const char *help_top = "";
    const char *help_bottom = "";
    const char *state_label = "EDIT";
    const char *horse_label = "HORSE";
    bool bettor_busted = bettor_is_busted(game.session.active_bettor);

    if(game.session.past_performance_active != 0u) {
        struct RaceEntry *entry = &race->entries[game.session.past_performance_entry];
        struct Horse *horse = &game.session.stable[entry->horse_index];

        set_font(FONT_NICETY_BOLD);
        text_update_pair_block(&betting_summary_block,
            "PAST PERFS",
            horse_color_name[horse->color]);
        set_font(FONT_NICETY);

        sprintf(title_line, "R%u %s %uF  %u-1",
            (unsigned)(game.session.current_race + 1u),
            surface_name[race->surface],
            (unsigned)race->distance_furlongs,
            (unsigned)entry->odds);
        sprintf(summary_line, "LAST %u RUNS", (unsigned)horse->history_count);
        text_update_pair_block(&betting_wager_block, title_line, summary_line);
        text_update_pair_block(&betting_help_block,
            "Y CLOSE L/R HORSE",
            "B RETURN   LAST 3");
        return;
    }

    if(game.session.flow_phase == RACE_FLOW_START_GATE) {
        sprintf(title_line, "AT GATE");
        sprintf(summary_line, "R%u %s %uF  %s",
            (unsigned)(game.session.current_race + 1u),
            surface_name[race->surface],
            (unsigned)race->distance_furlongs,
            jockey_control_label());

        set_font(FONT_NICETY_BOLD);
        text_update_pair_block(&betting_summary_block, title_line, summary_line);
        set_font(FONT_NICETY);
        update_jockey_block(&betting_wager_block,
            "GATE",
            jockey_horse_label(),
            "GO");
        text_update_pair_block(&betting_help_block,
            "START BREAK  B BACK",
            "A BREAK     GO RACE");
        return;
    }

    if(game.session.flow_phase == RACE_FLOW_JOCKEY_SETUP) {
        sprintf(title_line, "JOCKEY SET");
        sprintf(summary_line, "R%u %s %uF  %s",
            (unsigned)(game.session.current_race + 1u),
            surface_name[race->surface],
            (unsigned)race->distance_furlongs,
            jockey_control_label());

        set_font(FONT_NICETY_BOLD);
        text_update_pair_block(&betting_summary_block, title_line, summary_line);
        set_font(FONT_NICETY);
        update_jockey_block(&betting_wager_block,
            jockey_control_label(),
            jockey_horse_label(),
            "RACE");
        text_update_pair_block(&betting_help_block,
            "U/D CHOICE START GO",
            "A GO      B BETS");
        return;
    }

    strcpy(title_line, "ZEAL DOWNS");

    if(bettor_busted) {
        sprintf(summary_line, "R%u %s %uF B%u OUT",
            (unsigned)(game.session.current_race + 1u),
            surface_name[race->surface],
            (unsigned)race->distance_furlongs,
            (unsigned)(game.session.active_bettor + 1u));
        strcpy(amount_value, " --- ");
        strcpy(horse_value, "OUT");
        horse_label = "STATUS";
        state_label = "BUST";
    } else {
        sprintf(summary_line, "R%u %s %uF B%u $%u",
            (unsigned)(game.session.current_race + 1u),
            surface_name[race->surface],
            (unsigned)race->distance_furlongs,
            (unsigned)(game.session.active_bettor + 1u),
            (unsigned)bettor->bankroll);
    }

    if(bettor_busted) {
        help_top = game.session.betting_complete ? "START NEXT  L/R VIEW" : "WAIT NEXT BETTOR";
        help_bottom = "BUSTED OUT OF CASH";
    } else if(wager->type == BET_TYPE_NONE) {
        strcpy(amount_value, " --- ");
        strcpy(horse_value, "PASS");
        state_label = wager->confirmed ? "DONE" : "PASS";
    } else {
        uint8_t first_horse_index = race->entries[wager->first_pick].horse_index;

        sprintf(amount_value, "$%03u", (unsigned)wager->amount);

        if(wager->type == BET_TYPE_EXACTA) {
            uint8_t second_horse_index = race->entries[wager->second_pick].horse_index;
            sprintf(horse_value, "%s-%s",
                horse_color_short_name[game.session.stable[first_horse_index].color],
                horse_color_short_name[game.session.stable[second_horse_index].color]);
            horse_label = (game.session.selected_exacta_pick == 0u) ? "1ST" : "2ND";
        } else {
            sprintf(horse_value, "%s",
                horse_color_short_name[game.session.stable[first_horse_index].color]);
        }

        state_label = wager->confirmed ? "LOCK" : "EDIT";
    }

    if(bettor_busted) {
        if(game.session.betting_complete) {
            strcpy(title_line, "BETS LOCKED");
        }
    } else if(game.session.betting_complete) {
        strcpy(title_line, "BETS LOCKED");
        help_top = "START NEXT  B EDIT";
        help_bottom = "L/R VIEW  X TYPE";
    } else if(wager->type == BET_TYPE_EXACTA
        && game.session.selected_field == BETTING_FIELD_HORSE) {
        help_top = "U/D HORSE  A NEXT";
        help_bottom = "L/R FIELD  B RESET";
    } else {
        help_top = "L/R FIELD  A LOCK";
        help_bottom = "U/D EDIT SELECT PASS";
    }

    set_font(FONT_NICETY_BOLD);
    text_update_pair_block(&betting_summary_block, title_line, summary_line);
    set_font(FONT_NICETY);
    update_wager_block(&betting_wager_block,
        bet_type_name[wager->type],
        amount_value,
        horse_value,
        horse_label,
        game.session.betting_complete == 0u && game.session.selected_field == BETTING_FIELD_TYPE,
        game.session.betting_complete == 0u && game.session.selected_field == BETTING_FIELD_AMOUNT,
        game.session.betting_complete == 0u && game.session.selected_field == BETTING_FIELD_HORSE,
        state_label);
    text_update_pair_block(&betting_help_block, help_top, help_bottom);
}

void game_betting_refresh_text_blocks(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];

    game_ui_clear_overlay_layer();
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

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
    text_draw_strip_alloc(game_ui_centered_strip_x("ZEAL DOWNS"), 2,
        "ZEAL DOWNS", COL_YELLOW, COL_BLACK, 4, true);
    text_draw_strip_alloc(2, 5, "POST HORSE      ODDS", COL_LIGHT_GRAY, COL_BLACK, 4, true);
    text_init_pair_block(&betting_summary_block, 4, 2,
        BETTING_SUMMARY_SPAN, COL_WHITE, COL_BLACK);
    text_init_pair_block(&betting_wager_block, 4, 4,
        BETTING_WAGER_SPAN, COL_LIGHT_GRAY, COL_BLACK);
    text_init_pair_block(&betting_help_block, 1, 13,
        BETTING_WAGER_SPAN, COL_LIGHT_GRAY, COL_BLACK);
    update_betting_text_blocks();

    if(game.session.past_performance_active != 0u) {
        render_past_performance_rows();
        render_tilemap(1);
        return;
    }

    for(uint8_t i = 0; i < 4u; i++) {
        struct RaceEntry *entry = &race->entries[i];
        struct Horse *horse = &game.session.stable[entry->horse_index];

        draw_bet_entry(horse_icon_tile(horse->color), entry_marker_tile(i), (uint8_t)(7u + i),
            horse_color_name[horse->color], horse->color, entry->odds);
    }

    {
        char bank_line[20];
        sprintf(bank_line, "BANKROLL B%u $%u",
            (unsigned)(game.session.active_bettor + 1u),
            (unsigned)game.session.bettors[game.session.active_bettor].bankroll);
        text_draw_strip_alloc(game_ui_centered_strip_x(bank_line), 12, bank_line, COL_YELLOW, COL_BLACK, 4, true);
    }

    render_tilemap(1);
}

void game_betting_draw_template_screen(void)
{
    reset_layer_scrolls();
    game_ui_draw_board_background_layer();
    game_betting_refresh_text_blocks();
}