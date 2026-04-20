#ifndef GAME_RACE_H
#define GAME_RACE_H

#include <stdbool.h>
#include <stdint.h>

void game_race_init(void);
void game_race_input(uint8_t key, bool down);
void game_race_update(void);
void game_race_draw(void);

#endif