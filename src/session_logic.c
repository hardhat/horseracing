#include "session_logic.h"

#include <string.h>

#include "wager_math.h"

static uint8_t session_logic_pack_top_two_colors(const struct SessionState *session,
    const struct RaceCard *race)
{
    uint8_t winner_entry = race->result.finish_order[0];
    uint8_t runner_up_entry = race->result.finish_order[1];
    uint8_t winner_color = session->stable[race->entries[winner_entry].horse_index].color;
    uint8_t runner_up_color = session->stable[race->entries[runner_up_entry].horse_index].color;

    return (uint8_t)((winner_color << 4) | runner_up_color);
}

static void session_logic_append_past_performance(struct Horse *horse,
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

static uint8_t session_logic_selected_entry(const struct SessionState *session, uint8_t bettor_index)
{
    const struct Wager *wager = &session->wagers[bettor_index];

    if(wager->type == BET_TYPE_EXACTA && session->selected_exacta_pick != 0u) {
        return wager->second_pick;
    }

    return wager->first_pick;
}

static bool session_logic_bettor_can_wager(const struct SessionState *session, uint8_t bettor_index)
{
    const struct Bettor *bettor = &session->bettors[bettor_index];

    return bettor_index < session->bettor_count
        && bettor->active != 0u
        && bettor->bankroll != 0u;
}

static bool session_logic_find_next_pending_bettor(const struct SessionState *session,
    uint8_t start_index, uint8_t *bettor_index)
{
    for(uint8_t i = start_index; i < session->bettor_count; i++) {
        if(session_logic_bettor_can_wager(session, i) && session->wagers[i].confirmed == 0u) {
            *bettor_index = i;
            return true;
        }
    }

    return false;
}

static void session_logic_select_active_bettor(struct SessionState *session, uint8_t bettor_index)
{
    const struct Wager *wager = &session->wagers[bettor_index];

    session->active_bettor = bettor_index;
    session->selected_exacta_pick = 0u;
    session->selected_field = (wager->type == BET_TYPE_NONE)
        ? BETTING_FIELD_TYPE
        : BETTING_FIELD_HORSE;
    session->selected_entry = session_logic_selected_entry(session, bettor_index);
}

static void session_logic_finalize_broke_bettors(struct SessionState *session)
{
    for(uint8_t i = 0; i < session->bettor_count; i++) {
        if(!session_logic_bettor_can_wager(session, i)) {
            struct Wager *wager = &session->wagers[i];

            wager->type = BET_TYPE_NONE;
            wager->amount = 0u;
            wager->confirmed = 1u;
        }
    }
}

static void session_logic_init_betting_turn(struct SessionState *session)
{
    uint8_t next_bettor;

    session_logic_finalize_broke_bettors(session);

    if(session_logic_find_next_pending_bettor(session, 0u, &next_bettor)) {
        session->betting_complete = 0u;
        session_logic_select_active_bettor(session, next_bettor);
    } else {
        session->betting_complete = 1u;
    }
}

void session_logic_reset_wager(struct Wager *wager)
{
    wager->amount = 50u;
    wager->type = BET_TYPE_WIN;
    wager->first_pick = 0u;
    wager->second_pick = 1u;
    wager->locked_win_odds = 0u;
    wager->confirmed = 0u;
}

void session_logic_prepare_next_race(struct SessionState *session)
{
    session->flow_phase = RACE_FLOW_BETTING;
    session->betting_complete = 0u;
    session->selected_field = BETTING_FIELD_HORSE;
    session->selected_exacta_pick = 0u;
    session->selected_entry = 0u;
    session->past_performance_active = 0u;
    session->past_performance_entry = 0u;
    session->active_bettor = 0u;
    session->jockey_assignment = JOCKEY_ASSIGN_CPU;
    session->jockey_count = 0u;

    for(uint8_t i = 0; i < session->bettor_count; i++) {
        session->wagers[i].bettor_index = i;
        session_logic_reset_wager(&session->wagers[i]);
    }

    session_logic_init_betting_turn(session);
}

void session_logic_settle_race_wagers(struct SessionState *session)
{
    struct RaceCard *race = &session->races[session->current_race];

    for(uint8_t i = 0; i < session->bettor_count; i++) {
        uint16_t payout = wager_math_payout(&session->wagers[i], race);

        session->bettors[i].bankroll = (uint16_t)(session->bettors[i].bankroll + payout);
        session->bettors[i].active = (session->bettors[i].bankroll != 0u) ? 1u : 0u;
    }
}

void session_logic_record_race_result(struct SessionState *session,
    uint8_t race_index, const uint8_t finish_order[4], uint16_t winning_time)
{
    struct RaceCard *race = &session->races[race_index];
    uint8_t top_two_colors;

    for(uint8_t i = 0u; i < 4u; i++) {
        race->result.finish_order[i] = finish_order[i];
    }

    race->result.winning_time = winning_time;
    top_two_colors = session_logic_pack_top_two_colors(session, race);

    for(uint8_t i = 0u; i < 4u; i++) {
        uint8_t entry_index = race->result.finish_order[i];
        struct Horse *horse = &session->stable[race->entries[entry_index].horse_index];

        session_logic_append_past_performance(horse,
            race->distance_furlongs,
            race->surface,
            (uint8_t)(i + 1u),
            top_two_colors,
            race->result.winning_time);
    }
}

bool session_logic_advance_after_results(struct SessionState *session)
{
    if(session->current_race + 1u >= 10u) {
        return false;
    }

    session->current_race++;
    session_logic_prepare_next_race(session);
    return true;
}