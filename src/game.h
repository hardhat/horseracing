#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define TILES_ACROSS 20
#define TILES_DOWN 15

struct Game
{
    uint8_t menu_choice;
    uint8_t cursor_x;
    uint8_t cursor_y;
    uint16_t frame_counter;
};
extern struct Game game;

void init_game(void);
void resume_game(void);
void input_game(uint8_t key, bool down);
void update_game(void);
void draw_game(void);
void game_show_dialog(const char *text);

void init_gameover(void);
void input_gameover(uint8_t key, bool down);
void update_gameover(void);
void draw_gameover(void);

#endif