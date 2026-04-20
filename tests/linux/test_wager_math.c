#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "wager_math.h"

static struct RaceCard make_race(uint8_t win_odds)
{
    struct RaceCard race;

    memset(&race, 0, sizeof(race));
    race.entries[0].odds = win_odds;
    race.entries[1].odds = 5u;
    race.entries[2].odds = 7u;
    race.entries[3].odds = 9u;
    race.result.finish_order[0] = 0u;
    race.result.finish_order[1] = 2u;
    race.result.finish_order[2] = 1u;
    race.result.finish_order[3] = 3u;

    return race;
}

static void test_max_commit_amount_caps_at_250(void)
{
    assert(wager_math_max_commit_amount(0u) == 0u);
    assert(wager_math_max_commit_amount(125u) == 125u);
    assert(wager_math_max_commit_amount(250u) == 250u);
    assert(wager_math_max_commit_amount(750u) == 250u);
}

static void test_commit_clamps_to_bankroll_and_cap(void)
{
    struct RaceCard race = make_race(6u);
    struct Wager wager;
    uint16_t bankroll = 750u;

    memset(&wager, 0, sizeof(wager));
    wager.type = BET_TYPE_WIN;
    wager.amount = 400u;
    wager.first_pick = 0u;

    wager_math_commit_wager(&bankroll, &wager, &race);

    assert(wager.amount == 250u);
    assert(bankroll == 500u);
    assert(wager.confirmed == 1u);
}

static void test_commit_clamps_to_remaining_bankroll(void)
{
    struct RaceCard race = make_race(3u);
    struct Wager wager;
    uint16_t bankroll = 90u;

    memset(&wager, 0, sizeof(wager));
    wager.type = BET_TYPE_WIN;
    wager.amount = 120u;
    wager.first_pick = 0u;

    wager_math_commit_wager(&bankroll, &wager, &race);

    assert(wager.amount == 90u);
    assert(bankroll == 0u);
    assert(wager.confirmed == 1u);
}

static void test_commit_locks_displayed_win_odds(void)
{
    struct RaceCard race = make_race(6u);
    struct Wager wager;
    uint16_t bankroll = 200u;

    memset(&wager, 0, sizeof(wager));
    wager.type = BET_TYPE_WIN;
    wager.amount = 50u;
    wager.first_pick = 0u;

    wager_math_commit_wager(&bankroll, &wager, &race);
    race.entries[0].odds = 2u;

    assert(wager.locked_win_odds == 6u);
    assert(wager_math_payout(&wager, &race) == 350u);
}

static void test_win_payout_requires_the_winner(void)
{
    struct RaceCard race = make_race(4u);
    struct Wager wager;

    memset(&wager, 0, sizeof(wager));
    wager.type = BET_TYPE_WIN;
    wager.amount = 25u;
    wager.first_pick = 1u;
    wager.locked_win_odds = 4u;
    wager.confirmed = 1u;

    assert(wager_math_payout(&wager, &race) == 0u);
}

static void test_exacta_payout_requires_order(void)
{
    struct RaceCard race = make_race(6u);
    struct Wager win_exacta;
    struct Wager lose_exacta;

    memset(&win_exacta, 0, sizeof(win_exacta));
    memset(&lose_exacta, 0, sizeof(lose_exacta));

    win_exacta.type = BET_TYPE_EXACTA;
    win_exacta.amount = 10u;
    win_exacta.first_pick = 0u;
    win_exacta.second_pick = 2u;
    win_exacta.confirmed = 1u;

    lose_exacta = win_exacta;
    lose_exacta.first_pick = 2u;
    lose_exacta.second_pick = 0u;

    assert(wager_math_payout(&win_exacta, &race) == 160u);
    assert(wager_math_payout(&lose_exacta, &race) == 0u);
}

static void test_none_bet_confirms_without_changing_bankroll(void)
{
    struct RaceCard race = make_race(6u);
    struct Wager wager;
    uint16_t bankroll = 300u;

    memset(&wager, 0, sizeof(wager));
    wager.type = BET_TYPE_NONE;
    wager.amount = 99u;

    wager_math_commit_wager(&bankroll, &wager, &race);

    assert(bankroll == 300u);
    assert(wager.amount == 0u);
    assert(wager.locked_win_odds == 0u);
    assert(wager.confirmed == 1u);
}

static void test_settlement_total_matches_expected_bankrolls(void)
{
    struct RaceCard race = make_race(3u);
    struct Wager win_wager;
    struct Wager exacta_wager;
    uint16_t bankroll_a = 200u;
    uint16_t bankroll_b = 150u;

    memset(&win_wager, 0, sizeof(win_wager));
    memset(&exacta_wager, 0, sizeof(exacta_wager));

    win_wager.type = BET_TYPE_WIN;
    win_wager.amount = 50u;
    win_wager.first_pick = 0u;
    wager_math_commit_wager(&bankroll_a, &win_wager, &race);

    exacta_wager.type = BET_TYPE_EXACTA;
    exacta_wager.amount = 25u;
    exacta_wager.first_pick = 0u;
    exacta_wager.second_pick = 2u;
    wager_math_commit_wager(&bankroll_b, &exacta_wager, &race);

    bankroll_a = (uint16_t)(bankroll_a + wager_math_payout(&win_wager, &race));
    bankroll_b = (uint16_t)(bankroll_b + wager_math_payout(&exacta_wager, &race));

    assert(bankroll_a == 350u);
    assert(bankroll_b == 525u);
}

int main(void)
{
    test_max_commit_amount_caps_at_250();
    test_commit_clamps_to_bankroll_and_cap();
    test_commit_clamps_to_remaining_bankroll();
    test_commit_locks_displayed_win_odds();
    test_win_payout_requires_the_winner();
    test_exacta_payout_requires_order();
    test_none_bet_confirms_without_changing_bankroll();
    test_settlement_total_matches_expected_bankrolls();

    puts("wager_math tests passed");
    return 0;
}