#ifndef SESSION_LOGIC_H
#define SESSION_LOGIC_H

#include <stdbool.h>

#include "game.h"

void session_logic_reset_wager(struct Wager *wager);
void session_logic_prepare_next_race(struct SessionState *session);
void session_logic_settle_race_wagers(struct SessionState *session);
void session_logic_record_race_result(struct SessionState *session,
	uint8_t race_index, const uint8_t finish_order[4], uint16_t winning_time);
bool session_logic_advance_after_results(struct SessionState *session);

#endif