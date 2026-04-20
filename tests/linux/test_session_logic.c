#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "session_logic.h"
#include "wager_math.h"

static struct RaceCard make_race(uint8_t winner_odds)
{
    struct RaceCard race;

    memset(&race, 0, sizeof(race));
    race.entries[0].odds = winner_odds;
    race.entries[1].odds = 5u;
    race.entries[2].odds = 7u;
    race.entries[3].odds = 9u;
    race.result.finish_order[0] = 0u;
    race.result.finish_order[1] = 2u;
    race.result.finish_order[2] = 1u;
    race.result.finish_order[3] = 3u;

    return race;
}

static void test_settle_race_updates_multiple_bettors(void)
{
    struct SessionState session;

    memset(&session, 0, sizeof(session));
    session.bettor_count = 3u;
    session.current_race = 0u;
    session.races[0] = make_race(3u);

    session.bettors[0].bankroll = 200u;
    session.bettors[0].active = 1u;
    session.bettors[1].bankroll = 150u;
    session.bettors[1].active = 1u;
    session.bettors[2].bankroll = 40u;
    session.bettors[2].active = 1u;

    session.wagers[0].type = BET_TYPE_WIN;
    session.wagers[0].amount = 50u;
    session.wagers[0].first_pick = 0u;
    wager_math_commit_wager(&session.bettors[0].bankroll, &session.wagers[0], &session.races[0]);

    session.wagers[1].type = BET_TYPE_EXACTA;
    session.wagers[1].amount = 25u;
    session.wagers[1].first_pick = 0u;
    session.wagers[1].second_pick = 2u;
    wager_math_commit_wager(&session.bettors[1].bankroll, &session.wagers[1], &session.races[0]);

    session.wagers[2].type = BET_TYPE_WIN;
    session.wagers[2].amount = 40u;
    session.wagers[2].first_pick = 1u;
    wager_math_commit_wager(&session.bettors[2].bankroll, &session.wagers[2], &session.races[0]);

    session_logic_settle_race_wagers(&session);

    assert(session.bettors[0].bankroll == 350u);
    assert(session.bettors[1].bankroll == 525u);
    assert(session.bettors[2].bankroll == 0u);
    assert(session.bettors[0].active == 1u);
    assert(session.bettors[1].active == 1u);
    assert(session.bettors[2].active == 0u);
}

static void test_advance_after_results_prepares_next_betting_round(void)
{
    struct SessionState session;
    bool advanced;

    memset(&session, 0, sizeof(session));
    session.bettor_count = 3u;
    session.current_race = 0u;
    session.bettors[0].bankroll = 350u;
    session.bettors[0].active = 1u;
    session.bettors[1].bankroll = 0u;
    session.bettors[1].active = 0u;
    session.bettors[2].bankroll = 120u;
    session.bettors[2].active = 1u;

    advanced = session_logic_advance_after_results(&session);

    assert(advanced);
    assert(session.current_race == 1u);
    assert(session.flow_phase == RACE_FLOW_BETTING);
    assert(session.jockey_assignment == JOCKEY_ASSIGN_CPU);
    assert(session.jockey_count == 0u);
    assert(session.active_bettor == 0u);
    assert(session.selected_field == BETTING_FIELD_HORSE);
    assert(session.selected_entry == 0u);
    assert(session.betting_complete == 0u);

    assert(session.wagers[0].type == BET_TYPE_WIN);
    assert(session.wagers[0].amount == 50u);
    assert(session.wagers[0].confirmed == 0u);
    assert(session.wagers[1].type == BET_TYPE_NONE);
    assert(session.wagers[1].amount == 0u);
    assert(session.wagers[1].confirmed == 1u);
    assert(session.wagers[2].type == BET_TYPE_WIN);
    assert(session.wagers[2].amount == 50u);
    assert(session.wagers[2].confirmed == 0u);
}

static void test_final_race_does_not_advance(void)
{
    struct SessionState session;

    memset(&session, 0, sizeof(session));
    session.bettor_count = 2u;
    session.current_race = 9u;
    session.bettors[0].bankroll = 100u;
    session.bettors[0].active = 1u;

    assert(!session_logic_advance_after_results(&session));
    assert(session.current_race == 9u);
}

int main(void)
{
    test_settle_race_updates_multiple_bettors();
    test_advance_after_results_prepares_next_betting_round();
    test_final_race_does_not_advance();

    puts("session_logic tests passed");
    return 0;
}