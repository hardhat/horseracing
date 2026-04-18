
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
    bool type_selected, bool amount_selected, bool horse_selected,
    bool confirmed)
{
    clear_text_tiles(block->bg, block->span);

    draw_text(0, 0, "TYPE", COL_LIGHT_GRAY);
    draw_field_value(40, 0, 4, type_value, type_selected);
    draw_text(80, 0, "AMT", COL_LIGHT_GRAY);
    draw_field_value(112, 0, 5, amount_value, amount_selected);

    draw_text(0, 8, "HORSE", COL_LIGHT_GRAY);
    draw_field_value(48, 8, 7, horse_value, horse_selected);
    draw_text(120, 8, confirmed ? "LOCK" : "EDIT", COL_LIGHT_GRAY);

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
    game.session.selected_entry = 0;
    game.session.selected_field = BETTING_FIELD_HORSE;

    for(uint8_t i = 0; i < configured_bettors; i++) {
        game.session.bettors[i].bankroll = 750;
        game.session.bettors[i].active = 1;
    }

    for(uint8_t i = 0; i < HORSE_COLOR_COUNT; i++) {
        init_horse(&game.session.stable[i], i);
    }

    for(uint8_t i = 0; i < 6u; i++) {
        game.session.wagers[i].amount = 50;
        game.session.wagers[i].bettor_index = i;
        game.session.wagers[i].type = BET_TYPE_WIN;
        game.session.wagers[i].first_pick = 0;
        game.session.wagers[i].second_pick = 1;
        game.session.wagers[i].confirmed = 0;
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

static void refresh_betting_text_blocks(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
    struct Wager *wager = &game.session.wagers[game.session.active_bettor];
    char summary_line[32];
    char amount_value[8];
    char horse_value[12];
    uint8_t first_horse_index = race->entries[wager->first_pick].horse_index;

    sprintf(summary_line, "R%u %s %uF  $%u",
        (unsigned)(game.session.current_race + 1u),
        surface_name[race->surface],
        (unsigned)race->distance_furlongs,
        (unsigned)bettor->bankroll);

    sprintf(amount_value, "$%03u", (unsigned)wager->amount);

    if(wager->type == BET_TYPE_EXACTA) {
        uint8_t second_horse_index = race->entries[wager->second_pick].horse_index;
        sprintf(horse_value, "%s-%s",
            horse_color_short_name[game.session.stable[first_horse_index].color],
            horse_color_short_name[game.session.stable[second_horse_index].color]);
    } else {
        sprintf(horse_value, "%s",
            horse_color_short_name[game.session.stable[first_horse_index].color]);
    }

    set_font(FONT_NICETY_BOLD);
    text_update_pair_block(&betting_summary_block, "BET SHEET", summary_line);
    set_font(FONT_NICETY);
    update_wager_block(&betting_wager_block,
        bet_type_name[wager->type],
        amount_value,
        horse_value,
        game.session.selected_field == BETTING_FIELD_TYPE,
        game.session.selected_field == BETTING_FIELD_AMOUNT,
        game.session.selected_field == BETTING_FIELD_HORSE,
        wager->confirmed != 0);
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
    refresh_betting_text_blocks();

    for(uint8_t i = 0; i < 4; i++) {
        struct RaceEntry *entry = &race->entries[i];
        struct Horse *horse = &game.session.stable[entry->horse_index];

        sprintf(entry_line, "%-6s %u-1", horse_color_name[horse->color], (unsigned)entry->odds);
        draw_bet_entry(horse_icon_tile(horse->color), entry_marker_tile(i), (uint8_t)(7u + i), entry_line);
    }

    text_draw_pair_alloc(1, 13,
        "L/R FIELD A BET",
        "U/D EDIT  B BACK",
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
    memset(&game.session, 0, sizeof(game.session));

    if(game.configured_bettors == 0u) {
        game.configured_bettors = 1u;
    }

    init_generated_session();

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
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, 1);
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
            clear_confirmed_wager(bettor, wager);
            switch(game.session.selected_field) {
                case BETTING_FIELD_TYPE:
                    cycle_wager_type(wager, -1);
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
            if(game.session.selected_field > 0) {
                game.session.selected_field--;
                refresh_betting_text_blocks();
            }
            break;

        case INPUT_RIGHT:
            if(game.session.selected_field + 1u < BETTING_FIELD_COUNT) {
                game.session.selected_field++;
                refresh_betting_text_blocks();
            }
            break;

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
            refresh_betting_text_blocks();
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
            refresh_betting_text_blocks();
            break;
        }

        case INPUT_X:
        {
            struct Bettor *bettor = &game.session.bettors[game.session.active_bettor];
            struct Wager *wager = &game.session.wagers[game.session.active_bettor];
            clear_confirmed_wager(bettor, wager);
            cycle_wager_type(wager, 1);
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
    if(game.session.selected_field == BETTING_FIELD_HORSE) {
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
