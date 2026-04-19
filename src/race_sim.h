#ifndef RACE_SIM_H
#define RACE_SIM_H

#include <stdbool.h>
#include <stdint.h>

#define RACE_SIM_SURFACE_COUNT 3u

struct RaceSimTraits
{
    uint8_t break_speed;
    uint8_t early_pace;
    uint8_t endurance;
    uint8_t stretch_speed;
    uint8_t surface_affinity[RACE_SIM_SURFACE_COUNT];
};

struct RaceSimState
{
    uint8_t fatigue;
    uint8_t coax_boost;
    uint8_t whip_boost;
    uint8_t cpu_whip_uses;
    uint8_t cpu_whip_cooldown;
    uint16_t distance_units;
    uint8_t progress;
};

uint16_t race_sim_total_distance_units(uint8_t distance_furlongs);
uint8_t race_sim_phase_index(uint16_t covered_units, uint16_t total_units);
void race_sim_reset_state(struct RaceSimState *state);
void race_sim_advance_state(struct RaceSimState *state, uint8_t distance, uint16_t total_units);
uint8_t race_sim_step_distance(const struct RaceSimTraits *traits,
    uint8_t surface, uint8_t distance_furlongs,
    struct RaceSimState *state, bool cpu_controlled, bool coax, bool whip,
    uint16_t race_tick, uint16_t random_value);

#endif