#include "main.h"
#include "menu.h"
#include "game.h"
#include "dat.h"

#define TITLE_TILE 0x90
#define MENU_TILE 0x98
#define CHOICE_TILE 0xA0
#define CHOICE_TILE2 0xB0
#define CHOICE_TILE3 0xC0
#define ARROW_TILE 0xAF

#define MAX_CHOICE_TIMER 4

int choice=0;
int last_choice;
int choice_timer=MAX_CHOICE_TIMER;
int choice_color=COL_RED;   // Selected color, which rotates

void init_menu(void)
{
    char buffer[17+17];

    last_choice=-1;
    // Template sample: replace these menu entries with horse-racing setup flow.
    for(int i=0;i<17+17;i++) buffer[i]=TILE_COLOR_TITLE+i;
    for(int i=0;i<17+17;i+=17) draw_tilemap_array(2,2+(i/17),(uint8_t *)buffer+i,17);
    render_tilemap(0);
    clear_text_tiles(COL_DARK_GREEN, 7);
    set_font(FONT_NICETY_BOLD);
    draw_text_opaque(4, 4, " Press Start ", COL_WHITE, COL_BLUE);
    render_text(MENU_TILE, 7);
    for(int i=0;i<7;i++) {
        draw_tilemap(7+i, 7, MENU_TILE+i);
    }
    set_font(FONT_NICETY);
    clear_text_tiles(COL_BLACK, 1);
    draw_text(0,0,">",COL_RED);
    draw_text(0,0,"-",COL_RED);
    render_text(ARROW_TILE, 1);
    clear_text_tiles(COL_WHITE, 9);
    draw_text(8,4,"Play Demo",COL_DARK_GREEN);
    render_text(CHOICE_TILE, 9);
    clear_text_tiles(COL_WHITE, 9);
    draw_text(8,4,"Input Test",COL_DARK_GREEN);
    render_text(CHOICE_TILE2, 9);
    clear_text_tiles(COL_WHITE, 9);
    draw_text(8,4,"TODO",COL_DARK_GREEN);
    render_text(CHOICE_TILE3, 9);
    for(int i=0;i<9;i++) {
        draw_tilemap(7+i, 9, CHOICE_TILE+i);
        draw_tilemap(7+i, 10, CHOICE_TILE2+i);
        draw_tilemap(7+i, 11, CHOICE_TILE3+i);
    }
    render_tilemap(0);
}

void input_menu(uint8_t key, bool down)
{
    if(down && key == INPUT_START)
    {
    game.menu_choice=choice;
    set_state(GAME_STATE_GAME);
    }
    if(down && key == INPUT_UP)
    { 
        choice--;
        if(choice<0) choice=2;
    }
    if(down && key == INPUT_DOWN)
    {
        choice++;
        if(choice>2) choice=0;
    }
}

void update_menu(void)
{
    choice_timer--;
    if(choice_timer==0)
    {
        choice_timer=MAX_CHOICE_TIMER;
        choice_color=(choice_color==COL_RED)?COL_LIGHT_GRAY:COL_RED;
        last_choice=-1;
    }
}

void draw_menu(void)
{
    if(last_choice!=choice) {
        reset_sprite();
        add_sprite(7*16,choice*16+9*16+4,ARROW_TILE);
        //add_sprite(6*16,choice*16+9*16,TILE_PC_SWORDSMAN+choice);
        render_sprites();
        last_choice=choice;
    }
}