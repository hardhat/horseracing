
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "game_betting.h"
#include "game.h"
#include "game_race.h"
#include "game_ui.h"
#include "wager_math.h"
#include "session_logic.h"

struct Game game;

static bool held[MAX_INPUT];
static uint16_t session_rng = 0x43E7u;

static const uint8_t distance_groups[4][2] = {
    {3u, 4u},
    {5u, 6u},
    {7u, 8u},
    {9u, 10u},
};
uint16_t game_next_random_u16(void)
{
    session_rng = (uint16_t)(session_rng * 25173u + 13849u);
    return session_rng;
}

static uint8_t random_below(uint8_t limit)
{
    if(limit == 0u) {
        return 0u;
    }
    return (uint8_t)(game_next_random_u16() % limit);
}

static uint8_t random_range_u8(uint8_t min_value, uint8_t max_value)
{
    return (uint8_t)(min_value + random_below((uint8_t)(max_value - min_value + 1u)));
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

static void finalize_bettor_activity(void)
{
    for(uint8_t i = 0; i < game.session.bettor_count; i++) {
        game.session.bettors[i].active = (game.session.bettors[i].bankroll != 0u) ? 1u : 0u;
    }
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

void game_show_dialog(const char *text)
{
    // Sample hook only. Replace this with a proper modal, status banner, or message box.
    debug_logf("game: %s", text);
}

void resume_game(void)
{
    game_ui_clear_overlay_layer();
    render_tilemap(1);

    game_betting_draw_template_screen();
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
                game_betting_refresh_text_blocks();
                break;

            case INPUT_DOWN:
            case INPUT_LEFT:
                cycle_jockey_assignment(-1);
                game_betting_refresh_text_blocks();
                break;

            case INPUT_A:
            case INPUT_START:
                confirm_jockey_setup();
                break;

            case INPUT_B:
                cancel_jockey_setup();
                game_betting_refresh_text_blocks();
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
                game_betting_refresh_text_blocks();
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
                game_betting_refresh_text_blocks();
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
            game_betting_refresh_text_blocks();
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
            game_betting_refresh_text_blocks();
            break;
        }

        case INPUT_LEFT:
            if(game.session.betting_complete) {
                if(game.session.active_bettor > 0u) {
                    select_active_bettor((uint8_t)(game.session.active_bettor - 1u));
                    game_betting_refresh_text_blocks();
                }
            } else if(game.session.selected_field > 0) {
                game.session.selected_field--;
                game_betting_refresh_text_blocks();
            }
            break;

        case INPUT_RIGHT:
            if(game.session.betting_complete) {
                if(game.session.active_bettor + 1u < game.session.bettor_count) {
                    select_active_bettor((uint8_t)(game.session.active_bettor + 1u));
                    game_betting_refresh_text_blocks();
                }
            } else if(game.session.selected_field + 1u < BETTING_FIELD_COUNT) {
                game.session.selected_field++;
                game_betting_refresh_text_blocks();
            }
            break;

        case INPUT_SELECT:
            if(!game.session.betting_complete) {
                skip_active_bettor();
                game_betting_refresh_text_blocks();
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
            game_betting_refresh_text_blocks();
            break;
        }

        case INPUT_B:
            reset_active_wager();
            game_betting_refresh_text_blocks();
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
            game_betting_refresh_text_blocks();
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
    game_race_init();
}

void input_gameover(uint8_t key, bool down)
{
    game_race_input(key, down);
}

void update_gameover(void)
{
    game_race_update();
}

void draw_gameover(void)
{
    game_race_draw();
}
