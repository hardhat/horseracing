#ifndef MENU_H
#define MENU_H

#include<stdbool.h>
#include<stdint.h>

void init_menu(void);
void input_menu(uint8_t key, bool down);
void update_menu(void);
void draw_menu(void);

#endif