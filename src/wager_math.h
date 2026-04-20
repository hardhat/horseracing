#ifndef WAGER_MATH_H
#define WAGER_MATH_H

#include <stdint.h>

#include "game.h"

uint16_t wager_math_max_commit_amount(uint16_t bankroll);
uint16_t wager_math_clamp_commit_amount(uint16_t requested, uint16_t bankroll);
void wager_math_commit_wager(uint16_t *bankroll, struct Wager *wager, const struct RaceCard *race);
uint16_t wager_math_payout(const struct Wager *wager, const struct RaceCard *race);

#endif