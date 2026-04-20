
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "game.h"
#include "race_sim.h"
#include "text.h"
#include "wager_math.h"
#include "session_logic.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xFFu)
#define BETTING_SUMMARY_SPAN  ((uint8_t)12u)
#define BETTING_WAGER_SPAN    ((uint8_t)12u)

struct Game game;

static bool held[MAX_INPUT];
static uint16_t session_rng = 0x43E7u;
static bool race_held[MAX_INPUT];
static struct RaceSimState race_state[4];
static uint8_t race_finish_order[4];
static uint8_t race_finish_count;
static uint8_t race_controlled_entry;
static uint16_t race_tick;
static uint8_t race_show_results;
static uint8_t race_show_standings;
static uint16_t race_result_timer;
static uint8_t standings_order[6];
static uint8_t race_lane_cooldown[4];
static uint16_t race_camera_scroll_x;
static uint8_t race_furlongs_remaining;

static struct TextStripBlock race_furlong_block;

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

static const uint8_t distance_groups[4][2] = {
    {3u, 4u},
    {5u, 6u},
    {7u, 8u},
    {9u, 10u},
};

static const uint8_t race_lane_y[4] = { 4u, 6u, 8u, 10u };

#define RACE_RESULT_DELAY ((uint16_t)150u)
#define RACE_UNIT_PIXELS ((uint16_t)6u)
#define RACE_WORLD_MARGIN ((uint16_t)32u)
#define RACE_CAMERA_TARGET_X ((uint16_t)120u)
#define RACE_LANE_NEAR_PIXELS ((uint16_t)20u)
#define RACE_LANE_AHEAD_PIXELS ((uint16_t)36u)
#define RACE_LANE_SWITCH_DELAY ((uint8_t)8u)
#define RACE_FURLONG_UNITS ((uint16_t)8u)

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

static uint8_t centered_strip_x(const char *text)
{
    uint8_t span = text_measure_span(text);

    if(span >= TILES_ACROSS) {
        return 0u;
    }

    return (uint8_t)((TILES_ACROSS - span) >> 1u);
}

static uint8_t horse_text_color(uint8_t color)
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

static void clear_overlay_layer(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
}

static void draw_board_background_layer(void)
{
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    draw_board_shell();
    render_tilemap(0);
}

static bool bettor_is_busted(uint8_t bettor_index)
{
    return bettor_index < game.session.bettor_count
        && game.session.bettors[bettor_index].bankroll == 0u;
}

static void format_bettor_status(char *buffer, uint8_t bettor_index)
{
    if(bettor_is_busted(bettor_index)) {
        sprintf(buffer, "B%u OUT", (unsigned)(bettor_index + 1u));
        return;
    }

    sprintf(buffer, "B%u $%u",
        (unsigned)(bettor_index + 1u),
        (unsigned)game.session.bettors[bettor_index].bankroll);
}

static uint16_t race_total_units(void)
{
    return race_sim_total_distance_units(
        game.session.races[game.session.current_race].distance_furlongs);
}

static uint16_t race_entry_world_x(uint8_t entry_index)
{
    return (uint16_t)(RACE_WORLD_MARGIN + race_state[entry_index].distance_units * RACE_UNIT_PIXELS);
}

static uint16_t race_world_length_pixels(void)
{
    return (uint16_t)(race_total_units() * RACE_UNIT_PIXELS + RACE_WORLD_MARGIN + 32u);
}

static uint8_t leader_race_entry(void)
{
    uint8_t leader = 0u;

    for(uint8_t i = 1u; i < 4u; i++) {
        if(race_state[i].distance_units > race_state[leader].distance_units) {
            leader = i;
        }
    }

    return leader;
}

static bool lane_has_room(uint8_t entry_index, uint8_t lane)
{
    uint16_t own_x = race_entry_world_x(entry_index);

    for(uint8_t other = 0u; other < 4u; other++) {
        uint16_t other_x;

        if(other == entry_index) {
            continue;
        }
        if(game.session.races[game.session.current_race].entries[other].lane != lane) {
            continue;
        }

        other_x = race_entry_world_x(other);
        if(other_x > own_x) {
            if((uint16_t)(other_x - own_x) < RACE_LANE_NEAR_PIXELS) {
                return false;
            }
        } else if((uint16_t)(own_x - other_x) < RACE_LANE_NEAR_PIXELS) {
            return false;
        }
    }

    return true;
}

static bool lane_is_blocked_ahead(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;
    uint16_t own_x = race_entry_world_x(entry_index);

    for(uint8_t other = 0u; other < 4u; other++) {
        uint16_t other_x;

        if(other == entry_index) {
            continue;
        }
        if(game.session.races[game.session.current_race].entries[other].lane != lane) {
            continue;
        }

        other_x = race_entry_world_x(other);
        if(other_x > own_x && (uint16_t)(other_x - own_x) <= RACE_LANE_AHEAD_PIXELS) {
            return true;
        }
    }

    return false;
}

static void move_race_entry_lane(uint8_t entry_index, uint8_t next_lane)
{
    if(next_lane >= 4u) {
        return;
    }

    game.session.races[game.session.current_race].entries[entry_index].lane = next_lane;
    race_lane_cooldown[entry_index] = RACE_LANE_SWITCH_DELAY;
}

static void update_race_camera(void)
{
    uint16_t leader_x = race_entry_world_x(leader_race_entry());
    uint16_t max_scroll = race_world_length_pixels();

    if(max_scroll > 320u) {
        max_scroll = (uint16_t)(max_scroll - 320u);
    } else {
        max_scroll = 0u;
    }

    if(leader_x > RACE_CAMERA_TARGET_X) {
        race_camera_scroll_x = (uint16_t)(leader_x - RACE_CAMERA_TARGET_X);
        if(race_camera_scroll_x > max_scroll) {
            race_camera_scroll_x = max_scroll;
        }
    } else {
        race_camera_scroll_x = 0u;
    }

    set_layer_scroll(0u, 0u, 0u);
    set_layer_scroll(1u, (uint16_t)(race_camera_scroll_x >> 2), 0u);
}

static void update_race_furlong_status(void)
{
    uint16_t total_units = race_total_units();
    uint16_t leader_units = race_state[leader_race_entry()].distance_units;
    uint8_t remaining = 0u;
    char line[8];

    if(total_units > leader_units) {
        remaining = (uint8_t)((total_units - leader_units + (RACE_FURLONG_UNITS - 1u)) / RACE_FURLONG_UNITS);
    }

    if(remaining == race_furlongs_remaining) {
        return;
    }

    race_furlongs_remaining = remaining;
    if(remaining == 0u) {
        strcpy(line, "HOME");
    } else {
        sprintf(line, "%uF GO", (unsigned)remaining);
    }
    text_update_strip_block(&race_furlong_block, line);
    render_tilemap(0);
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
    const char *horse_name, uint8_t horse_color, uint8_t odds)
{
    draw_tilemap(1, map_y, icon_tile);
    draw_tilemap(2, map_y, marker_tile);
    text_draw_strip_alloc(4, map_y, horse_name, horse_text_color(horse_color), COL_BLACK, 4, false);

    {
        char line[8];
        sprintf(line, "%u-1", (unsigned)odds);
        text_draw_strip_alloc(14, map_y, line, COL_YELLOW, COL_BLACK, 4, false);
    }
}

static void assign_race_odds(struct RaceCard *race);

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
        wager->locked_win_odds = 0u;
        wager->confirmed = 0;
        assign_race_odds(&game.session.races[game.session.current_race]);
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
    session_logic_reset_wager(wager);
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
    static const uint8_t tentative_odds_by_rank[4] = { 3u, 4u, 5u, 6u };
    static const uint8_t live_odds_by_rank[4] = { 2u, 3u, 5u, 7u };
    uint8_t order[4] = { 0u, 1u, 2u, 3u };
    uint16_t score[4];
    uint16_t support[4] = { 0u, 0u, 0u, 0u };
    uint8_t active_race = (race == &game.session.races[game.session.current_race]) ? 1u : 0u;
    uint8_t any_confirmed = 0u;

    for(uint8_t i = 0; i < 4u; i++) {
        score[i] = projected_horse_score(&game.session.stable[race->entries[i].horse_index],
            race->surface,
            race->distance_furlongs);
    }

    if(active_race != 0u) {
        for(uint8_t i = 0; i < game.session.bettor_count; i++) {
            const struct Wager *wager = &game.session.wagers[i];

            if(wager->confirmed == 0u || wager->type == BET_TYPE_NONE) {
                continue;
            }

            any_confirmed = 1u;
            if(wager->type == BET_TYPE_WIN) {
                support[wager->first_pick] = (uint16_t)(support[wager->first_pick] + wager->amount * 2u);
            } else if(wager->type == BET_TYPE_EXACTA) {
                support[wager->first_pick] = (uint16_t)(support[wager->first_pick] + wager->amount);
                support[wager->second_pick] = (uint16_t)(support[wager->second_pick] + wager->amount);
            }
        }

        if(any_confirmed != 0u) {
            for(uint8_t i = 0; i < 4u; i++) {
                score[i] = (uint16_t)(score[i] + support[i]);
            }
        }
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
        race->entries[order[i]].odds = (any_confirmed != 0u)
            ? live_odds_by_rank[i]
            : tentative_odds_by_rank[i];
    }
}

static uint8_t pack_top_two_colors(const struct RaceCard *race)
{
    uint8_t winner_entry = race->result.finish_order[0];
    uint8_t runner_up_entry = race->result.finish_order[1];
    uint8_t winner_color = game.session.stable[race->entries[winner_entry].horse_index].color;
    uint8_t runner_up_color = game.session.stable[race->entries[runner_up_entry].horse_index].color;

    return (uint8_t)((winner_color << 4) | runner_up_color);
}

static void append_past_performance(struct Horse *horse,
    uint8_t distance_furlongs, uint8_t surface, uint8_t finish_position,
    uint8_t top_two_colors, uint16_t finish_time)
{
    struct PastPerformance performance;

    performance.distance_furlongs = distance_furlongs;
    performance.surface = surface;
    performance.finish_position = finish_position;
    performance.top2 = top_two_colors;
    performance.finish_time = finish_time;

    if(horse->history_count < 3u) {
        horse->history[horse->history_count] = performance;
        horse->history_count++;
        return;
    }

    memmove(&horse->history[0], &horse->history[1], sizeof(horse->history[0]) * 2u);
    horse->history[2] = performance;
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
    game.session.past_performance_active = 0u;
    game.session.past_performance_entry = 0u;
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
    wager_math_commit_wager(&bettor->bankroll, wager,
        &game.session.races[game.session.current_race]);
    assign_race_odds(&game.session.races[game.session.current_race]);
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

static void begin_start_gate(void)
{
    game.session.flow_phase = RACE_FLOW_START_GATE;
}

static void cancel_jockey_setup(void)
{
    game.session.flow_phase = RACE_FLOW_BETTING;
}

static void confirm_jockey_setup(void)
{
    game.session.jockey_count = (game.session.jockey_assignment == JOCKEY_ASSIGN_CPU) ? 0u : 1u;
    begin_start_gate();
}

static void cancel_start_gate(void)
{
    game.session.flow_phase = RACE_FLOW_JOCKEY_SETUP;
}

static void confirm_start_gate(void)
{
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

static uint8_t surface_track_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_CLEAR_0;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_CLEAR_0;

        // case SURFACE_DRY:
        default:
            return TILE_TRACK_DRY_CLEAR_0;
    }
}

static uint8_t surface_backdrop_tile(uint8_t surface, uint8_t column)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return (column & 1u) == 0u ? TILE_TRACK_MUD_GRANDSTAND_0 : TILE_TRACK_MUD_CROWD_0;

        case SURFACE_TURF:
            return (column & 1u) == 0u ? TILE_TRACK_TURF_GRANDSTAND_0 : TILE_TRACK_TURF_CROWD_0;

        default:
            return (column & 1u) == 0u ? TILE_TRACK_DRY_GRANDSTAND_0 : TILE_TRACK_DRY_CROWD_0;
    }
}

static uint8_t surface_furlong_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_FURLONG;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_FURLONG;

        default:
            return TILE_TRACK_DRY_FURLONG;
    }
}

static uint8_t surface_finish_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_FINISH;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_FINISH;

        // case SURFACE_DRY:
        default:
            return TILE_TRACK_DRY_FINISH;
    }
}

static uint8_t controlled_race_entry(void)
{
    switch(game.session.jockey_assignment) {
        case JOCKEY_ASSIGN_OUTSIDE_1:
            return 0u;

        case JOCKEY_ASSIGN_OUTSIDE_4:
            return 3u;

        // case JOCKEY_ASSIGN_CPU:
        default:
            return 0xFFu;
    }
}

static void prepare_next_race(void)
{
    game.session.flow_phase = RACE_FLOW_BETTING;
    game.session.betting_complete = 0u;
    game.session.selected_field = BETTING_FIELD_HORSE;
    game.session.selected_exacta_pick = 0u;
    game.session.selected_entry = 0u;
    game.session.past_performance_active = 0u;
    game.session.past_performance_entry = 0u;
    game.session.active_bettor = 0u;
    game.session.jockey_assignment = JOCKEY_ASSIGN_CPU;
    game.session.jockey_count = 0u;

    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        game.session.wagers[i].bettor_index = i;
        reset_wager(&game.session.wagers[i]);
    }

    init_betting_turn();
}

static void clear_current_race_wagers(void)
{
    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        clear_confirmed_wager(&game.session.bettors[i], &game.session.wagers[i]);
        reset_wager(&game.session.wagers[i]);
        game.session.wagers[i].bettor_index = i;
    }

    assign_race_odds(&game.session.races[game.session.current_race]);
}

static void skip_ahead_races(uint8_t count)
{
    uint8_t target_race;

    if(game.session.current_race >= 9u || count == 0u) {
        return;
    }

    clear_current_race_wagers();

    target_race = (uint8_t)(game.session.current_race + count);
    if(target_race > 9u) {
        target_race = 9u;
    }

    game.session.current_race = target_race;
    prepare_next_race();
    resume_game();
}

static uint16_t wager_payout(const struct Wager *wager, const struct RaceCard *race)
{
    return wager_math_payout(wager, race);
}

static void finalize_bettor_activity(void)
{
    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        game.session.bettors[i].active = (game.session.bettors[i].bankroll != 0u) ? 1u : 0u;
    }
}

static void build_standings_order(void)
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

static void settle_race_wagers(void)
{
    session_logic_settle_race_wagers(&game.session);
}

static void record_race_result(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    uint8_t top_two_colors;

    for(uint8_t i = 0; i < 4u; i++) {
        race->result.finish_order[i] = race_finish_order[i];
    }

    race->result.winning_time = (uint16_t)(600u + race_tick * 3u);
    top_two_colors = pack_top_two_colors(race);

    for(uint8_t i = 0; i < 4u; i++) {
        uint8_t entry_index = race->result.finish_order[i];
        struct Horse *horse = &game.session.stable[race->entries[entry_index].horse_index];

        append_past_performance(horse,
            race->distance_furlongs,
            race->surface,
            (uint8_t)(i + 1u),
            top_two_colors,
            race->result.winning_time);
    }
}

static void render_race_results_screen(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    char line[20];
    char status_line[12];

    reset_layer_scrolls();
    draw_board_background_layer();
    clear_overlay_layer();
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

    text_draw_strip_alloc(centered_strip_x("OFFICIAL RESULTS"), 2,
        "OFFICIAL RESULTS", COL_YELLOW, COL_BLACK, 4, true);
    set_font(FONT_NICETY_BOLD);
    sprintf(line, "R%u  WIN %u",
        (unsigned)(game.session.current_race + 1u),
        (unsigned)race->result.winning_time);
    text_draw_strip_alloc(centered_strip_x(line), 3, line, COL_WHITE, COL_BLACK, 4, true);
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
            horse_text_color(horse->color), COL_BLACK, 4, false);
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

static void render_final_standings_screen(void)
{
    char line[20];
    char status_line[12];
    uint8_t winner_index;

    reset_layer_scrolls();
    draw_board_background_layer();
    clear_overlay_layer();
    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);

    build_standings_order();
    winner_index = standings_order[0];

    text_draw_strip_alloc(centered_strip_x("FINAL STANDINGS"), 2,
        "FINAL STANDINGS", COL_YELLOW, COL_BLACK, 4, true);
    set_font(FONT_NICETY_BOLD);

    format_bettor_status(status_line, winner_index);
    sprintf(line, "WINNER %s", status_line);
    text_draw_strip_alloc(centered_strip_x(line), 3, line, COL_YELLOW, COL_BLACK, 4, true);
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

static void finish_race_and_show_results(void)
{
    if(race_show_results != 0u) {
        return;
    }

    record_race_result();
    settle_race_wagers();
    race_show_results = 1u;
    race_result_timer = RACE_RESULT_DELAY;
    clear_sprites();
    render_race_results_screen();
}

static void advance_after_results(void)
{
    if(session_logic_advance_after_results(&game.session)) {
        game.resume_session = 1u;
        set_state(GAME_STATE_GAME);
        return;
    }

    race_show_standings = 1u;
    race_show_results = 0u;
    clear_sprites();
    render_final_standings_screen();
}

static void build_race_backdrop_layer(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    for(uint8_t x = 0u; x < TILES_ACROSS; x++) {
        draw_tilemap(x, 2u, surface_backdrop_tile(race->surface, x));
        draw_tilemap(x, 3u, surface_backdrop_tile(race->surface, (uint8_t)(x + 1u)));
    }
    render_tilemap(1);
}

static void build_race_foreground_layer(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    uint8_t track_tile = surface_track_tile(race->surface);

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    fill_tilemap(TILE_TRACK_UTIL_DRY_0, 0, 2u, TILES_ACROSS, 1u);
    fill_tilemap(TILE_TRACK_UTIL_RAIL_CURVE_0, 0, 12u, TILES_ACROSS, 1u);

    for(uint8_t lane = 0u; lane < 4u; lane++) {
        uint8_t y = race_lane_y[lane];

        fill_tilemap(track_tile, 0, y, TILES_ACROSS, 1u);
        draw_tilemap(0, y, entry_marker_tile(lane));
        draw_tilemap(19, y, surface_finish_tile(race->surface));
    }

    render_tilemap(0);
}

static void draw_race_world_sprites(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    uint16_t total_units = race_total_units();
    uint16_t finish_x = (uint16_t)(RACE_WORLD_MARGIN + total_units * RACE_UNIT_PIXELS);

    reset_sprite();

    for(uint8_t marker = 1u; marker < race->distance_furlongs; marker++) {
        uint16_t marker_units = (uint16_t)(marker * RACE_FURLONG_UNITS);
        uint16_t marker_x = (uint16_t)(RACE_WORLD_MARGIN + marker_units * RACE_UNIT_PIXELS);

        if(marker_x + 16u >= race_camera_scroll_x && marker_x < race_camera_scroll_x + 304u) {
            add_sprite((uint16_t)(marker_x - race_camera_scroll_x), 40u, surface_furlong_tile(race->surface));
        }
    }

    if(finish_x + 16u >= race_camera_scroll_x && finish_x < race_camera_scroll_x + 304u) {
        add_sprite((uint16_t)(finish_x - race_camera_scroll_x), 40u,
            surface_finish_tile(race->surface));
    }

    for(uint8_t i = 0u; i < 4u; i++) {
        uint16_t horse_x = race_entry_world_x(i);
        uint8_t lane = race->entries[i].lane;
        struct Horse *horse = &game.session.stable[race->entries[i].horse_index];

        if(horse_x + 16u < race_camera_scroll_x || horse_x > race_camera_scroll_x + 304u) {
            continue;
        }

        add_sprite((uint16_t)(horse_x - race_camera_scroll_x),
            (uint8_t)(race_lane_y[lane] * 16u),
            horse_icon_tile(horse->color));
    }

    render_sprites();
}

static void cycle_past_performance_entry(int8_t delta)
{
    uint8_t next = game.session.past_performance_entry;

    if(delta > 0) {
        next = (uint8_t)((next + 1u) & 0x03u);
    } else if(delta < 0) {
        next = (uint8_t)((next + 3u) & 0x03u);
    }

    game.session.past_performance_entry = next;
}

static void open_past_performance_view(void)
{
    game.session.past_performance_active = 1u;
    game.session.past_performance_entry = game.session.selected_entry & 0x03u;
}

static void close_past_performance_view(void)
{
    game.session.past_performance_active = 0u;
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

static void advance_race_entry(uint8_t entry_index, uint8_t distance)
{
    uint16_t total_units = race_sim_total_distance_units(
        game.session.races[game.session.current_race].distance_furlongs);

    if(race_state[entry_index].distance_units >= total_units) {
        return;
    }

    race_sim_advance_state(&race_state[entry_index], distance, total_units);

    if(race_state[entry_index].distance_units >= total_units) {
        race_finish_order[race_finish_count] = entry_index;
        race_finish_count++;
    }
}

static void update_controlled_lane(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;

    if(race_lane_cooldown[entry_index] != 0u) {
        race_lane_cooldown[entry_index]--;
        return;
    }

    if(race_held[INPUT_UP] && lane > 0u && lane_has_room(entry_index, (uint8_t)(lane - 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane - 1u));
        return;
    }

    if(race_held[INPUT_DOWN] && lane + 1u < 4u && lane_has_room(entry_index, (uint8_t)(lane + 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane + 1u));
    }
}

static void update_cpu_lane(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;
    bool blocked;

    if(race_lane_cooldown[entry_index] != 0u) {
        race_lane_cooldown[entry_index]--;
        return;
    }

    blocked = lane_is_blocked_ahead(entry_index);
    if(blocked && lane + 1u < 4u && lane_has_room(entry_index, (uint8_t)(lane + 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane + 1u));
        return;
    }

    if(lane > 0u && lane_has_room(entry_index, (uint8_t)(lane - 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane - 1u));
    }
}

static void update_race_lanes(void)
{
    for(uint8_t i = 0u; i < 4u; i++) {
        if(i == race_controlled_entry) {
            update_controlled_lane(i);
        } else {
            update_cpu_lane(i);
        }
    }
}

static uint8_t race_step_distance(uint8_t entry_index)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Horse *horse = &game.session.stable[race->entries[entry_index].horse_index];
    struct RaceSimTraits traits;
    uint8_t distance;
    uint8_t lane = race->entries[entry_index].lane;

    traits.break_speed = horse->traits.break_speed;
    traits.early_pace = horse->traits.early_pace;
    traits.endurance = horse->traits.endurance;
    traits.stretch_speed = horse->traits.stretch_speed;
    for(uint8_t surface = 0u; surface < SURFACE_COUNT; surface++) {
        traits.surface_affinity[surface] = horse->traits.surface_affinity[surface];
    }

    distance = race_sim_step_distance(&traits,
        race->surface,
        race->distance_furlongs,
        &race_state[entry_index],
        entry_index != race_controlled_entry,
        entry_index == race_controlled_entry && race_held[INPUT_Y],
        entry_index == race_controlled_entry && race_held[INPUT_B],
        race_tick,
        next_random_u16());

    if(lane == 0u && (race_tick & 0x0Fu) == 0u) {
        distance++;
    } else if(lane == 1u && (race_tick & 0x1Fu) == 0u) {
        distance++;
    } else if(lane == 3u && distance > 1u && (race_tick & 0x1Fu) == 0u) {
        distance--;
    }

    return distance;
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

static void refresh_betting_text_blocks(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];

    clear_overlay_layer();
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
    text_draw_strip_alloc(centered_strip_x("ZEAL DOWNS"), 2,
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

    for(uint8_t i = 0; i < 4; i++) {
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
        text_draw_strip_alloc(centered_strip_x(bank_line), 12, bank_line, COL_YELLOW, COL_BLACK, 4, true);
    }

    render_tilemap(1);
}

static void draw_template_screen(void)
{
    reset_layer_scrolls();
    draw_board_background_layer();
    refresh_betting_text_blocks();
}

void game_show_dialog(const char *text)
{
    // Sample hook only. Replace this with a proper modal, status banner, or message box.
    debug_logf("game: %s", text);
}

void resume_game(void)
{
    clear_overlay_layer();
    render_tilemap(1);

    draw_template_screen();
}

void init_game(void)
{
    if(game.resume_session != 0u) {
        debug_log("game: resume betting screen");
        memset(held, 0, sizeof(held));
        game.resume_session = 0u;
        game.cursor_x = 0;
        game.cursor_y = 0;
        prepare_next_race();
        clear_sprites();
        resume_game();
        return;
    }

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

    if(game.session.flow_phase == RACE_FLOW_START_GATE) {
        switch(key) {
            case INPUT_A:
            case INPUT_START:
                confirm_start_gate();
                break;

            case INPUT_B:
                cancel_start_gate();
                refresh_betting_text_blocks();
                break;

            default:
                break;
        }
        return;
    }

    if(game.session.past_performance_active != 0u) {
        if(!down) {
            return;
        }

        switch(key) {
            case INPUT_Y:
            case INPUT_B:
                close_past_performance_view();
                resume_game();
                break;

            case INPUT_LEFT:
            case INPUT_L:
                cycle_past_performance_entry(-1);
                resume_game();
                break;

            case INPUT_RIGHT:
            case INPUT_R:
                cycle_past_performance_entry(1);
                resume_game();
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

        case INPUT_R:
            skip_ahead_races(8u);
            break;

        case INPUT_Y:
            open_past_performance_view();
            resume_game();
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
    reset_sprite();
    if(game.session.past_performance_active == 0u
        && game.session.betting_complete == 0u
        && game.session.selected_field == BETTING_FIELD_HORSE) {
        add_sprite(48, (uint8_t)(selected_row_y() * 16u), TILE_CURSOR);
    }
    render_sprites();
}

void init_gameover(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    char header_line[24];

    reset_layer_scrolls();
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);

    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);
    memset(race_held, 0, sizeof(race_held));
    for(uint8_t i = 0u; i < 4u; i++) {
        race_sim_reset_state(&race_state[i]);
    }
    memset(race_finish_order, 0xFF, sizeof(race_finish_order));
    race_finish_count = 0u;
    race_controlled_entry = controlled_race_entry();
    race_tick = 0u;
    race_show_results = 0u;
    race_show_standings = 0u;
    race_result_timer = 0u;
    race_camera_scroll_x = 0u;
    race_furlongs_remaining = 0xFFu;
    memset(race_lane_cooldown, 0, sizeof(race_lane_cooldown));

    for(uint8_t i = 0u; i < 4u; i++) {
        race->entries[i].lane = i;
    }

    build_race_backdrop_layer();
    build_race_foreground_layer();

    sprintf(header_line, "R%u %s %uF",
        (unsigned)(game.session.current_race + 1u),
        surface_name[race->surface],
        (unsigned)race->distance_furlongs);

    set_font(FONT_NICETY_BOLD);
    text_draw_strip_alloc(2, 0, "RACE PROTOTYPE", COL_WHITE, COL_BLACK, 4, false);
    set_font(FONT_NICETY);
    text_draw_strip_alloc(5, 1, header_line, COL_LIGHT_GRAY, COL_BLACK, 4, false);
    text_init_strip_block(&race_furlong_block, 13, 1, 4u, COL_YELLOW, COL_BLACK, 4u, false);
    if(race_controlled_entry == 0xFFu) {
        text_draw_pair_alloc(3, 13,
            "CPU RACE",
            "START EXIT",
            COL_LIGHT_GRAY, COL_BLACK);
    } else {
        text_draw_pair_alloc(2, 13,
            "U/D LANE Y/B GO",
            "START EXIT",
            COL_LIGHT_GRAY, COL_BLACK);
    }

    render_tilemap(0);
    update_race_camera();
    update_race_furlong_status();
    clear_sprites();
    debug_log("game: race prototype screen");
}

void input_gameover(uint8_t key, bool down)
{
    if(race_show_standings != 0u) {
        if(down && (key == INPUT_START || key == INPUT_A)) {
            set_state(GAME_STATE_MENU);
        }
        return;
    }

    if(race_show_results != 0u) {
        return;
    }

    if(key < MAX_INPUT) {
        race_held[key] = down;
    }

    if(down && key == INPUT_START) {
        set_state(GAME_STATE_MENU);
    }
}

void update_gameover(void)
{
    if(race_show_standings != 0u) {
        return;
    }

    if(race_show_results != 0u) {
        if(race_result_timer > 0u) {
            race_result_timer--;
        }

        if(race_result_timer == 0u) {
            advance_after_results();
        }
        return;
    }

    race_tick++;
    update_race_lanes();
    update_race_camera();
    update_race_furlong_status();

    if((race_tick & 0x07u) != 0u || race_finish_count >= 4u) {
        if(race_finish_count >= 4u) {
            finish_race_and_show_results();
        }
        return;
    }

    for(uint8_t i = 0; i < 4u; i++) {
        advance_race_entry(i, race_step_distance(i));
    }
    update_race_camera();
    update_race_furlong_status();

    if(race_finish_count >= 4u) {
        finish_race_and_show_results();
    }
}

void draw_gameover(void)
{
    if(race_show_results == 0u && race_show_standings == 0u) {
        draw_race_world_sprites();
    }
}
