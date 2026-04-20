#include "wager_math.h"

uint16_t wager_math_max_commit_amount(uint16_t bankroll)
{
    return (bankroll > 250u) ? 250u : bankroll;
}

uint16_t wager_math_clamp_commit_amount(uint16_t requested, uint16_t bankroll)
{
    uint16_t max_amount = wager_math_max_commit_amount(bankroll);

    if(requested > max_amount) {
        return max_amount;
    }

    return requested;
}

void wager_math_commit_wager(uint16_t *bankroll, struct Wager *wager, const struct RaceCard *race)
{
    uint16_t amount;

    if(wager->type == BET_TYPE_NONE) {
        wager->amount = 0u;
        wager->locked_win_odds = 0u;
        wager->confirmed = 1u;
        return;
    }

    amount = wager_math_clamp_commit_amount(wager->amount, *bankroll);
    if(amount == 0u) {
        return;
    }

    wager->amount = amount;
    if(wager->type == BET_TYPE_WIN) {
        wager->locked_win_odds = race->entries[wager->first_pick].odds;
    } else {
        wager->locked_win_odds = 0u;
    }

    *bankroll = (uint16_t)(*bankroll - wager->amount);
    wager->confirmed = 1u;
}

uint16_t wager_math_payout(const struct Wager *wager, const struct RaceCard *race)
{
    if(wager->confirmed == 0u || wager->type == BET_TYPE_NONE) {
        return 0u;
    }

    if(wager->type == BET_TYPE_WIN) {
        uint8_t winner = race->result.finish_order[0];

        if(wager->first_pick == winner) {
            return (uint16_t)(wager->amount * (uint16_t)(wager->locked_win_odds + 1u));
        }
        return 0u;
    }

    if(wager->type == BET_TYPE_EXACTA
        && wager->first_pick == race->result.finish_order[0]
        && wager->second_pick == race->result.finish_order[1]) {
        return (uint16_t)(wager->amount * 16u);
    }

    return 0u;
}