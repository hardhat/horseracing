#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define TILES_ACROSS 20
#define TILES_DOWN 15

enum HorseColor
{
    HORSE_COLOR_WHITE,
    HORSE_COLOR_GREEN,
    HORSE_COLOR_GOLD,
    HORSE_COLOR_RED,
    HORSE_COLOR_BLUE,
    HORSE_COLOR_PURPLE,
    HORSE_COLOR_BROWN,
    HORSE_COLOR_BLACK,
    HORSE_COLOR_COUNT,
};

enum RaceSurface
{
    SURFACE_DRY,
    SURFACE_MUDDY,
    SURFACE_TURF,
    SURFACE_COUNT,
};

enum BetType
{
    BET_TYPE_NONE,
    BET_TYPE_WIN,
    BET_TYPE_EXACTA,
};

enum BettingField
{
    BETTING_FIELD_TYPE,
    BETTING_FIELD_AMOUNT,
    BETTING_FIELD_HORSE,
    BETTING_FIELD_COUNT,
};

enum RaceFlowPhase
{
    RACE_FLOW_BETTING,
    RACE_FLOW_JOCKEY_SETUP,
    RACE_FLOW_START_GATE,
};

enum JockeyAssignment
{
    JOCKEY_ASSIGN_CPU,
    JOCKEY_ASSIGN_OUTSIDE_1,
    JOCKEY_ASSIGN_OUTSIDE_4,
    JOCKEY_ASSIGN_COUNT,
};

struct HorseTraits
{
    uint8_t break_speed;
    uint8_t early_pace;
    uint8_t endurance;
    uint8_t stretch_speed;
    uint8_t surface_affinity[SURFACE_COUNT];
};

struct PastPerformance
{
    uint8_t distance_furlongs;
    uint8_t surface;
    uint8_t finish_position;
    uint8_t top2;
    uint16_t finish_time;
};

struct Horse
{
    uint8_t color;
    struct HorseTraits traits;
    struct PastPerformance history[3];
    uint8_t history_count;
};

struct Bettor
{
    uint16_t bankroll;
    uint8_t active;
};

struct Wager
{
    uint16_t amount;
    uint8_t bettor_index;
    uint8_t type;
    uint8_t first_pick;
    uint8_t second_pick;
    uint8_t locked_win_odds;
    uint8_t confirmed;
};

struct RaceEntry
{
    uint8_t horse_index;
    uint8_t odds;
    uint8_t lane;
};

struct RaceResult
{
    uint8_t finish_order[4];
    uint16_t winning_time;
};

struct RaceCard
{
    uint8_t surface;
    uint8_t distance_furlongs;
    struct RaceEntry entries[4];
    struct RaceResult result;
};

struct SessionState
{
    struct Bettor bettors[6];
    struct Horse stable[8];
    struct RaceCard races[10];
    struct Wager wagers[6];
    uint8_t bettor_count;
    uint8_t jockey_count;
    uint8_t current_race;
    uint8_t active_bettor;
    uint8_t betting_complete;
    uint8_t selected_entry;
    uint8_t selected_exacta_pick;
    uint8_t selected_field;
    uint8_t past_performance_active;
    uint8_t past_performance_entry;
    uint8_t flow_phase;
    uint8_t jockey_assignment;
};

struct Game
{
    uint8_t menu_choice;
    uint8_t configured_bettors;
    uint8_t resume_session;
    uint8_t cursor_x;
    uint8_t cursor_y;
    uint16_t frame_counter;
    struct SessionState session;
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