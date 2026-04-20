#ifndef SESSION_LOGIC_H
#define SESSION_LOGIC_H

#include <stdbool.h>

#include "game.h"

void session_logic_reset_wager(struct Wager *wager);
void session_logic_prepare_next_race(struct SessionState *session);
void session_logic_settle_race_wagers(struct SessionState *session);
bool session_logic_advance_after_results(struct SessionState *session);

#endif