
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "game.h"
#include "text.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xFFu)
#define BETTING_SUMMARY_SPAN  ((uint8_t)10u)
#define BETTING_WAGER_SPAN    ((uint8_t)12u)

struct Game game;

static bool held[MAX_INPUT];
static uint16_t session_rng = 0x43E7u;

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

static const char *const bet_type_name[] = {
    "NONE",
    "WIN",
    "EXA",
};

static const uint8_t distance_groups[4][2] = {
    {3u, 4u},
    {5u, 6u},
    {7u, 8u},
    {9u, 10u},
};

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

static uint16_t next_random_u16(void)
{
    session_rng = (uint16_t)(session_rng * 25173u + 13849u);
    return session_rng;
}

static uint8_t random_below(uint8_t limit)
{
    if(limit == 0u) {
        return 0u;
    }
    return (uint8_t)(next_random_u16() % limit);
}

static uint8_t random_range_u8(uint8_t min_value, uint8_t max_value)
{
    return (uint8_t)(min_value + random_below((uint8_t)(max_value - min_value + 1u)));
}

static void draw_field_value(uint16_t x, uint8_t y, uint8_t width,
    const char *value, bool selected)
{
    char padded[10];

    copy_padded_text(padded, width, value);
    draw_text_opaque(x, y, padded,
        COL_WHITE,
        selected ? COL_DARK_BLUE : COL_BLACK);
}

static void update_wager_block(const struct TextPairBlock *block,
    const char *type_value, const char *amount_value, const char *horse_value,
    const char *horse_label,
    bool type_selected, bool amount_selected, bool horse_selected,
    const char *state_label)
{
    clear_text_tiles(block->bg, block->span);

    draw_text(0, 0, "TYPE", COL_LIGHT_GRAY);
    draw_field_value(40, 0, 4, type_value, type_selected);
    draw_text(80, 0, "AMT", COL_LIGHT_GRAY);
    draw_field_value(112, 0, 5, amount_value, amount_selected);

    draw_text(0, 8, horse_label, COL_LIGHT_GRAY);
    draw_field_value(48, 8, 7, horse_value, horse_selected);
    draw_text(120, 8, state_label, COL_LIGHT_GRAY);

    render_text(block->base_tile, block->span);
}

static void update_jockey_block(const struct TextPairBlock *block,
    const char *control_value, const char *horse_value, const char *state_label)
{
    clear_text_tiles(block->bg, block->span);

    draw_text(0, 0, "CTRL", COL_LIGHT_GRAY);
    draw_field_value(40, 0, 4, control_value, true);
    draw_text(80, 0, "PHASE", COL_LIGHT_GRAY);
    draw_text(120, 0, state_label, COL_LIGHT_GRAY);

    draw_text(0, 8, "HORSE", COL_LIGHT_GRAY);
    draw_field_value(48, 8, 7, horse_value, true);

    render_text(block->base_tile, block->span);
}

static void draw_bet_entry(uint8_t icon_tile, uint8_t marker_tile, uint8_t map_y,
    const char *text)
{
    draw_tilemap(1, map_y, icon_tile);
    draw_tilemap(2, map_y, marker_tile);
    text_draw_strip_alloc(4, map_y, text, COL_WHITE, COL_BLACK, 4, false);
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

static void sync_selected_exacta_pick(struct Wager *wager)
{
    if(wager->type == BET_TYPE_EXACTA && game.session.selected_exacta_pick != 0u) {
        game.session.selected_entry = wager->second_pick;
    } else {
        game.session.selected_entry = wager->first_pick;
    }
}

static void reset_wager(struct Wager *wager)
{
    wager->amount = 50u;
    wager->type = BET_TYPE_WIN;
    wager->first_pick = 0u;
    wager->second_pick = 1u;
    wager->confirmed = 0u;
}

static void sync_selected_entry(void)
{
    sync_selected_exacta_pick(&game.session.wagers[game.session.active_bettor]);
}

static void normalize_wager(struct Wager *wager)
{
    if(wager->first_pick > 3u) {
        wager->first_pick = 0;
    }

    if(wager->type == BET_TYPE_NONE) {
        wager->amount = 0u;
        wager->second_pick = 0u;
        return;
    }

    if(wager->amount < 10u) {
        wager->amount = 10u;
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

    if(wager->type == BET_TYPE_NONE) {
        return;
    }

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
        wager->type = (wager->type == BET_TYPE_EXACTA) ? BET_TYPE_NONE : (uint8_t)(wager->type + 1u);
    } else if(delta < 0) {
        wager->type = (wager->type == BET_TYPE_NONE) ? BET_TYPE_EXACTA : (uint8_t)(wager->type - 1u);
    }
    normalize_wager(wager);
}

static void cycle_selected_horse(struct Wager *wager, int8_t delta)
{
    uint8_t *selected_pick;

    if(wager->type == BET_TYPE_NONE) {
        return;
    }

    selected_pick = (wager->type == BET_TYPE_EXACTA && game.session.selected_exacta_pick != 0u)
        ? &wager->second_pick
        : &wager->first_pick;

    if(delta > 0) {
        *selected_pick = (uint8_t)((*selected_pick + 1u) & 0x03u);
    } else if(delta < 0) {
        *selected_pick = (uint8_t)((*selected_pick + 3u) & 0x03u);
    }
    normalize_wager(wager);
    sync_selected_exacta_pick(wager);
}

static void toggle_exacta_pick(struct Wager *wager)
{
    if(wager->type != BET_TYPE_EXACTA) {
        game.session.selected_exacta_pick = 0u;
        sync_selected_exacta_pick(wager);
        return;
    }

    game.session.selected_exacta_pick ^= 1u;
    sync_selected_exacta_pick(wager);
}

static void sync_wager_selection_state(struct Wager *wager)
{
    if(wager->type != BET_TYPE_EXACTA) {
        game.session.selected_exacta_pick = 0u;
    }
    sync_selected_exacta_pick(wager);
}

static void seed_next_session(void)
{
    session_rng ^= (uint16_t)(session_rng << 7);
    session_rng ^= (uint16_t)(session_rng >> 9);
    session_rng ^= 0x9E37u;
    if(session_rng == 0u) {
        session_rng = 0x43E7u;
    }
}

static void init_horse(struct Horse *horse, uint8_t color)
{
    uint8_t style = random_below(3u);
    uint8_t stamina_bias = random_below(3u);

    memset(horse, 0, sizeof(*horse));
    horse->color = color;

    horse->traits.break_speed = random_range_u8(5u, 10u);
    horse->traits.early_pace = random_range_u8(5u, 10u);
    horse->traits.endurance = random_range_u8(5u, 10u);
    horse->traits.stretch_speed = random_range_u8(5u, 10u);

    if(style == 0u) {
        horse->traits.break_speed = random_range_u8(9u, 12u);
        horse->traits.early_pace = random_range_u8(8u, 12u);
    } else if(style == 1u) {
        horse->traits.endurance = random_range_u8(8u, 12u);
        horse->traits.stretch_speed = random_range_u8(8u, 12u);
    } else {
        horse->traits.break_speed = random_range_u8(6u, 11u);
        horse->traits.stretch_speed = random_range_u8(6u, 11u);
    }

    if(stamina_bias == 0u) {
        horse->traits.endurance = random_range_u8(9u, 12u);
    } else if(stamina_bias == 1u) {
        horse->traits.surface_affinity[SURFACE_MUDDY] = random_range_u8(9u, 12u);
    }

    for(uint8_t surface = 0; surface < SURFACE_COUNT; surface++) {
        if(horse->traits.surface_affinity[surface] == 0u) {
            horse->traits.surface_affinity[surface] = random_range_u8(5u, 12u);
        }
    }
}

static uint16_t projected_horse_score(const struct Horse *horse, uint8_t surface, uint8_t distance_furlongs)
{
    uint16_t score = horse->traits.surface_affinity[surface];

    score = (uint16_t)(score + horse->traits.early_pace + horse->traits.stretch_speed);

    if(distance_furlongs <= 4u) {
        score = (uint16_t)(score + horse->traits.break_speed + (horse->traits.early_pace >> 1));
    } else if(distance_furlongs <= 6u) {
        score = (uint16_t)(score + horse->traits.break_speed + horse->traits.endurance);
    } else if(distance_furlongs <= 8u) {
        score = (uint16_t)(score + horse->traits.endurance + horse->traits.stretch_speed);
    } else {
        score = (uint16_t)(score + (horse->traits.endurance << 1) + horse->traits.stretch_speed);
    }

    return score;
}

static void choose_race_entries(struct RaceCard *race)
{
    uint8_t shuffled[HORSE_COLOR_COUNT];

    for(uint8_t i = 0; i < HORSE_COLOR_COUNT; i++) {
        shuffled[i] = i;
    }

    for(uint8_t i = 0; i < HORSE_COLOR_COUNT; i++) {
        uint8_t swap_index = (uint8_t)(i + random_below((uint8_t)(HORSE_COLOR_COUNT - i)));
        uint8_t tmp = shuffled[i];
        shuffled[i] = shuffled[swap_index];
        shuffled[swap_index] = tmp;
    }

    for(uint8_t i = 0; i < 4u; i++) {
        race->entries[i].horse_index = shuffled[i];
        race->entries[i].lane = i;
    }
}

static void assign_race_odds(struct RaceCard *race)
{
    static const uint8_t odds_by_rank[4] = { 3u, 4u, 5u, 6u };
    uint8_t order[4] = { 0u, 1u, 2u, 3u };
    uint16_t score[4];

    for(uint8_t i = 0; i < 4u; i++) {
        score[i] = projected_horse_score(&game.session.stable[race->entries[i].horse_index],
            race->surface,
            race->distance_furlongs);
    }

    for(uint8_t i = 0; i < 3u; i++) {
        for(uint8_t j = (uint8_t)(i + 1u); j < 4u; j++) {
            uint8_t left = order[i];
            uint8_t right = order[j];

            if(score[right] > score[left]
                || (score[right] == score[left]
                    && race->entries[right].horse_index < race->entries[left].horse_index)) {
                uint8_t tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    for(uint8_t i = 0; i < 4u; i++) {
        race->entries[order[i]].odds = odds_by_rank[i];
    }
}

static void init_generated_session(void)
{
    uint8_t configured_bettors;

    memset(&game.session, 0, sizeof(game.session));
    seed_next_session();

    configured_bettors = game.configured_bettors;
    if(configured_bettors == 0u || configured_bettors > 6u) {
        configured_bettors = 1u;
    }

    game.session.bettor_count = configured_bettors;
    game.session.jockey_count = 0;
    game.session.current_race = 0;
    game.session.active_bettor = 0;
    game.session.betting_complete = 0;
    game.session.selected_entry = 0;
    game.session.selected_exacta_pick = 0;
    game.session.selected_field = BETTING_FIELD_HORSE;
    game.session.flow_phase = RACE_FLOW_BETTING;
    game.session.jockey_assignment = JOCKEY_ASSIGN_CPU;

    for(uint8_t i = 0; i < configured_bettors; i++) {
        game.session.bettors[i].bankroll = 750;
        game.session.bettors[i].active = 1;
    }

    for(uint8_t i = 0; i < HORSE_COLOR_COUNT; i++) {
        init_horse(&game.session.stable[i], i);
    }

    for(uint8_t i = 0; i < 6u; i++) {
        game.session.wagers[i].bettor_index = i;
        reset_wager(&game.session.wagers[i]);
    }

    for(uint8_t race_index = 0; race_index < 10u; race_index++) {
        uint8_t group_index = random_below(4u);
        struct RaceCard *race = &game.session.races[race_index];

        memset(race, 0, sizeof(*race));
        race->surface = random_below(SURFACE_COUNT);
        race->distance_furlongs = distance_groups[group_index][random_below(2u)];

        choose_race_entries(race);
        assign_race_odds(race);
    }
}

static void commit_wager(struct Bettor *bettor, struct Wager *wager)
{
    uint16_t max_amount = bettor->bankroll;

    if(wager->type == BET_TYPE_NONE) {
        wager->amount = 0u;
        wager->confirmed = 1u;
        return;
    }

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
    wager->confirmed = 1u;
}

static bool bettor_can_wager(uint8_t bettor_index)
{
    const struct Bettor *bettor = &game.session.bettors[bettor_index];

    return bettor_index < game.session.bettor_count
        && bettor->active != 0u
        && bettor->bankroll != 0u;
}

static bool find_next_pending_bettor(uint8_t start_index, uint8_t *bettor_index)
{
    for(uint8_t i = start_index; i < game.session.bettor_count; i++) {
        if(bettor_can_wager(i) && game.session.wagers[i].confirmed == 0u) {
            *bettor_index = i;
            return true;
        }
    }

    return false;
}

static void finalize_broke_bettors(void)
{
    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        if(!bettor_can_wager(i)) {
            struct Wager *wager = &game.session.wagers[i];

            wager->type = BET_TYPE_NONE;
            wager->amount = 0u;
            wager->confirmed = 1u;
        }
    }
}

static void select_active_bettor(uint8_t bettor_index)
{
    struct Wager *wager = &game.session.wagers[bettor_index];

    game.session.active_bettor = bettor_index;
    game.session.selected_exacta_pick = 0u;
    game.session.selected_field = (wager->type == BET_TYPE_NONE)
        ? BETTING_FIELD_TYPE
        : BETTING_FIELD_HORSE;
    sync_selected_entry();
}

static void advance_to_next_bettor(void)
{
    uint8_t next_bettor;

    finalize_broke_bettors();

    if(find_next_pending_bettor((uint8_t)(game.session.active_bettor + 1u), &next_bettor)) {
        game.session.betting_complete = 0u;
        select_active_bettor(next_bettor);
        return;
    }

    if(find_next_pending_bettor(0u, &next_bettor)) {
        game.session.betting_complete = 0u;
        select_active_bettor(next_bettor);
        return;
    }

    game.session.betting_complete = 1u;
}

static void reset_active_wager(void)
{
    struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];

    if(!bettor_can_wager(game.session.active_bettor)) {
        wager->type = BET_TYPE_NONE;
        wager->amount = 0u;
        wager->confirmed = 1u;
        return;
    }

    clear_confirmed_wager(bettor, wager);
    reset_wager(wager);
    game.session.selected_exacta_pick = 0u;
    game.session.betting_complete = 0u;
    sync_selected_entry();
}

static void skip_active_bettor(void)
{
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];

    reset_active_wager();
    wager->type = BET_TYPE_NONE;
    wager->amount = 0u;
    commit_wager(&game.session.bettors[game.session.active_bettor], wager);
    advance_to_next_bettor();
}

static void init_betting_turn(void)
{
    uint8_t next_bettor;

    finalize_broke_bettors();

    if(find_next_pending_bettor(0u, &next_bettor)) {
        game.session.betting_complete = 0u;
        select_active_bettor(next_bettor);
    } else {
        game.session.betting_complete = 1u;
    }
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

        // case JOCKEY_ASSIGN_CPU:
        default:
            return "NONE";
    }
}

static void cycle_jockey_assignment(int8_t delta)
{
    uint8_t next = game.session.jockey_assignment;

    if(delta > 0) {
        next = (uint8_t)(next + 1u);
        if(next >= JOCKEY_ASSIGN_COUNT) {
            next = JOCKEY_ASSIGN_CPU;
        }
    } else if(delta < 0) {
        next = (next == JOCKEY_ASSIGN_CPU)
            ? (uint8_t)(JOCKEY_ASSIGN_COUNT - 1u)
            : (uint8_t)(next - 1u);
    }

    game.session.jockey_assignment = next;
}

static void begin_jockey_setup(void)
{
    game.session.flow_phase = RACE_FLOW_JOCKEY_SETUP;
}

static void cancel_jockey_setup(void)
{
    game.session.flow_phase = RACE_FLOW_BETTING;
}

static void confirm_jockey_setup(void)
{
    game.session.jockey_count = (game.session.jockey_assignment == JOCKEY_ASSIGN_CPU) ? 0u : 1u;
    set_state(GAME_STATE_GAMEOVER);
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

static void refresh_betting_text_blocks(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];
    char title_line[16];
    char summary_line[32];
    char amount_value[8];
    char horse_value[12];
    const char *help_top;
    const char *help_bottom;
    const char *state_label;
    const char *horse_label = "HORSE";

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

    sprintf(title_line, "BETTOR %u/%u",
        (unsigned)(game.session.active_bettor + 1u),
        (unsigned)game.session.bettor_count);

    sprintf(summary_line, "R%u %s %uF  $%u",
        (unsigned)(game.session.current_race + 1u),
        surface_name[race->surface],
        (unsigned)race->distance_furlongs,
        (unsigned)bettor->bankroll);

    if(wager->type == BET_TYPE_NONE) {
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

    if(game.session.betting_complete) {
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

static void draw_template_screen(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    char entry_line[16];

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

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

    text_init_pair_block(&betting_summary_block, 3, 3,
        BETTING_SUMMARY_SPAN, COL_WHITE, COL_BLACK);
    text_init_pair_block(&betting_wager_block, 4, 6,
        BETTING_WAGER_SPAN, COL_LIGHT_GRAY, COL_BLACK);
    text_init_pair_block(&betting_help_block, 1, 13,
        BETTING_WAGER_SPAN, COL_LIGHT_GRAY, COL_BLACK);
    refresh_betting_text_blocks();

    for(uint8_t i = 0; i < 4; i++) {
        struct RaceEntry *entry = &race->entries[i];
        struct Horse *horse = &game.session.stable[entry->horse_index];

        sprintf(entry_line, "%-6s %u-1", horse_color_name[horse->color], (unsigned)entry->odds);
        draw_bet_entry(horse_icon_tile(horse->color), entry_marker_tile(i), (uint8_t)(7u + i), entry_line);
    }

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
    memset(&game.session, 0, sizeof(game.session));

    if(game.configured_bettors == 0u) {
        game.configured_bettors = 1u;
    }

    init_generated_session();
    init_betting_turn();

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

    if(game.session.flow_phase == RACE_FLOW_JOCKEY_SETUP) {
        switch(key) {
            case INPUT_UP:
            case INPUT_RIGHT:
                cycle_jockey_assignment(1);
                refresh_betting_text_blocks();
                break;

            case INPUT_DOWN:
            case INPUT_LEFT:
                cycle_jockey_assignment(-1);
                refresh_betting_text_blocks();
                break;

            case INPUT_A:
            case INPUT_START:
                confirm_jockey_setup();
                break;

            case INPUT_B:
                cancel_jockey_setup();
                refresh_betting_text_blocks();
                break;

            default:
                break;
        }
        return;
    }

    switch(key) {
        case INPUT_START:
            if(game.session.betting_complete) {
                begin_jockey_setup();
                refresh_betting_text_blocks();
            }
            break;

        case INPUT_UP:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            if(game.session.betting_complete) {
                break;
            }
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, 1);
                    sync_wager_selection_state(wager);
                    break;
                case BETTING_FIELD_AMOUNT:
                    adjust_wager_amount(bettor, wager, 1);
                    break;
                case BETTING_FIELD_HORSE:
                    cycle_selected_horse(wager, -1);
                    break;
            }
            refresh_betting_text_blocks();
            break;
        }

        case INPUT_DOWN:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            if(game.session.betting_complete) {
                break;
            }
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, -1);
                    sync_wager_selection_state(wager);
                    break;
                case BETTING_FIELD_AMOUNT:
                    adjust_wager_amount(bettor, wager, -1);
                    break;
                case BETTING_FIELD_HORSE:
                    cycle_selected_horse(wager, 1);
                    break;
            }
            refresh_betting_text_blocks();
            break;
        }

        case INPUT_LEFT:
            if(game.session.betting_complete) {
                if(game.session.active_bettor > 0u) {
                    select_active_bettor((uint8_t)(game.session.active_bettor - 1u));
                    refresh_betting_text_blocks();
                }
            } else if(game.session.selected_field > 0) {
                game.session.selected_field--;
                refresh_betting_text_blocks();
            }
            break;

        case INPUT_RIGHT:
            if(game.session.betting_complete) {
                if(game.session.active_bettor + 1u < game.session.bettor_count) {
                    select_active_bettor((uint8_t)(game.session.active_bettor + 1u));
                    refresh_betting_text_blocks();
                }
            } else if(game.session.selected_field + 1u < BETTING_FIELD_COUNT) {
                game.session.selected_field++;
                refresh_betting_text_blocks();
            }
            break;

        case INPUT_SELECT:
            if(!game.session.betting_complete) {
                skip_active_bettor();
                refresh_betting_text_blocks();
            }
            break;

        case INPUT_A:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            if(game.session.betting_complete) {
                break;
            }
            if(wager->confirmed == 0u) {
                if(wager->type == BET_TYPE_EXACTA
                    && game.session.selected_field == BETTING_FIELD_HORSE
                    && game.session.selected_exacta_pick == 0u) {
                    toggle_exacta_pick(wager);
                } else {
                    commit_wager(bettor, wager);
                    advance_to_next_bettor();
                }
            }
            refresh_betting_text_blocks();
            break;
        }

        case INPUT_B:
            reset_active_wager();
            refresh_betting_text_blocks();
            break;

        case INPUT_X:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            if(game.session.betting_complete) {
                break;
            }
            clear_confirmed_wager(bettor, wager);
            cycle_wager_type(wager, 1);
            sync_wager_selection_state(wager);
            refresh_betting_text_blocks();
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
    reset_sprite();
    if(game.session.betting_complete == 0u
        && game.session.selected_field == BETTING_FIELD_HORSE) {
        add_sprite(0, (uint8_t)(selected_row_y() * 16u), TILE_CURSOR);
    }
    render_sprites();
}

void init_gameover(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);

    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

    for(uint8_t y = 0; y < TILES_DOWN; y++) {
        for(uint8_t x = 0; x < TILES_ACROSS; x++) {
            draw_tilemap(x, y, (uint8_t)(TILE_COLOR_WHITE + ((x + y) & 0x03u)));
        }
    }

    set_font(FONT_NICETY_BOLD);
    text_draw_strip_alloc(6, 5, "TODO", COL_RED, COL_DARK_RED, 4, false);
    set_font(FONT_NICETY);
    text_draw_strip_alloc(2, 8, "replace with game-over/results", COL_WHITE, COL_BLACK, 4, false);

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
