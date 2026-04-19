#include <stddef.h>

#include "race_sim.h"

#define RACE_DISTANCE_SCALE ((uint16_t)8u)

static uint8_t race_sim_display_progress(uint16_t covered_units, uint16_t total_units)
{
    uint16_t scaled;

    if(total_units == 0u) {
        return 0u;
    }

    scaled = (uint16_t)((covered_units * 16u) / total_units);
    if(scaled > 16u) {
        scaled = 16u;
    }
    return (uint8_t)scaled;
}

uint16_t race_sim_total_distance_units(uint8_t distance_furlongs)
{
    return (uint16_t)(distance_furlongs * RACE_DISTANCE_SCALE);
}

uint8_t race_sim_phase_index(uint16_t covered_units, uint16_t total_units)
{
    uint16_t quarter;

    if(total_units == 0u) {
        return 0u;
    }

    quarter = (uint16_t)(total_units >> 2);
    if(quarter == 0u) {
        quarter = 1u;
    }

    if(covered_units < quarter) {
        return 0u;
    }
    if(covered_units < (uint16_t)(quarter * 2u)) {
        return 1u;
    }
    if(covered_units < (uint16_t)(quarter * 3u)) {
        return 2u;
    }
    return 3u;
}

void race_sim_reset_state(struct RaceSimState *state)
{
    if(state == NULL) {
        return;
    }

    state->fatigue = 0u;
    state->coax_boost = 0u;
    state->whip_boost = 0u;
    state->cpu_whip_uses = 0u;
    state->cpu_whip_cooldown = 0u;
    state->distance_units = 0u;
    state->progress = 0u;
}

static bool race_sim_should_cpu_whip(const struct RaceSimTraits *traits,
    const struct RaceSimState *state, uint8_t distance_furlongs,
    uint16_t random_value)
{
    uint16_t total_units = race_sim_total_distance_units(distance_furlongs);
    uint8_t phase = race_sim_phase_index(state->distance_units, total_units);
    uint8_t late_score;

    if(phase < 2u || state->cpu_whip_uses >= 2u || state->cpu_whip_cooldown != 0u) {
        return false;
    }
    if(state->whip_boost != 0u) {
        return false;
    }
    if(state->fatigue >= (uint8_t)(traits->endurance + 18u)) {
        return false;
    }

    late_score = (uint8_t)(traits->stretch_speed + (traits->endurance >> 1u));
    if(phase >= 3u) {
        return true;
    }

    return (uint8_t)(late_score + (random_value & 0x03u)) >= 13u;
}

void race_sim_advance_state(struct RaceSimState *state, uint8_t distance, uint16_t total_units)
{
    if(state == NULL || state->distance_units >= total_units) {
        return;
    }

    while(distance-- != 0u && state->distance_units < total_units) {
        state->distance_units++;
    }

    state->progress = race_sim_display_progress(state->distance_units, total_units);

    if(state->distance_units >= total_units) {
        state->distance_units = total_units;
        state->progress = 16u;
    }
}

uint8_t race_sim_step_distance(const struct RaceSimTraits *traits,
    uint8_t surface, uint8_t distance_furlongs,
    struct RaceSimState *state, bool cpu_controlled, bool coax, bool whip,
    uint16_t race_tick, uint16_t random_value)
{
    uint16_t total_units;
    uint8_t phase;
    uint8_t fatigue_gain;
    int16_t pace_score;
    uint8_t distance = 1u;
    bool active_whip;

    if(traits == NULL || state == NULL) {
        return distance;
    }

    total_units = race_sim_total_distance_units(distance_furlongs);
    phase = race_sim_phase_index(state->distance_units, total_units);
    active_whip = whip;

    if(state->cpu_whip_cooldown > 0u) {
        state->cpu_whip_cooldown--;
    }

    if(cpu_controlled && !coax && !whip) {
        active_whip = race_sim_should_cpu_whip(traits, state, distance_furlongs, random_value);
        if(active_whip) {
            state->cpu_whip_uses++;
            state->cpu_whip_cooldown = 8u;
        }
    }

    switch(phase) {
        case 0u:
            pace_score = (int16_t)(traits->break_speed * 2u + traits->early_pace + traits->endurance);
            break;

        case 1u:
            pace_score = (int16_t)(traits->early_pace * 2u + traits->endurance + traits->stretch_speed);
            break;

        case 2u:
            pace_score = (int16_t)(traits->early_pace + traits->endurance * 2u + traits->stretch_speed);
            break;

        default:
            pace_score = (int16_t)(traits->endurance + traits->stretch_speed * 2u);
            break;
    }

    if(distance_furlongs <= 4u) {
        pace_score = (int16_t)(pace_score
            + traits->break_speed * 2u
            + traits->early_pace
            - (traits->endurance >> 1));
    } else if(distance_furlongs >= 8u) {
        pace_score = (int16_t)(pace_score
            + (traits->endurance << 1)
            + traits->stretch_speed
            - traits->break_speed);
    }

    if(surface < RACE_SIM_SURFACE_COUNT) {
        pace_score = (int16_t)(pace_score + traits->surface_affinity[surface] - 8);
    }

    pace_score = (int16_t)(pace_score - state->fatigue);
    pace_score = (int16_t)(pace_score + state->coax_boost + state->whip_boost);
    pace_score = (int16_t)(pace_score + (int16_t)(random_value & 0x03u));

    if(coax) {
        if(state->coax_boost < 3u) {
            state->coax_boost++;
        }
        pace_score = (int16_t)(pace_score + 2 + state->coax_boost);
    } else if(state->coax_boost > 0u) {
        state->coax_boost--;
    }

    if(active_whip) {
        state->whip_boost = (phase >= 2u) ? 5u : 3u;
        pace_score = (int16_t)(pace_score + state->whip_boost);
        pace_score = (phase >= 2u)
            ? (int16_t)(pace_score + 3)
            : (int16_t)(pace_score + 1);
    } else if(state->whip_boost > 0u) {
        state->whip_boost--;
    }

    if(pace_score >= 26) {
        distance++;
    }
    if(pace_score >= 34) {
        distance++;
    }
    if(pace_score >= 42) {
        distance++;
    }

    fatigue_gain = (uint8_t)(1u + distance);
    if(distance_furlongs <= 4u && fatigue_gain > 1u) {
        fatigue_gain--;
    }
    if(distance_furlongs >= 7u) {
        fatigue_gain++;
    }
    if(surface == 1u) {
        fatigue_gain = (uint8_t)(fatigue_gain + 2u);
    }
    if(phase == 0u && fatigue_gain > 1u) {
        fatigue_gain--;
    }
    if(traits->endurance >= 10u && fatigue_gain > 1u) {
        fatigue_gain--;
    }
    if(coax) {
        fatigue_gain++;
    }
    if(active_whip) {
        fatigue_gain = (phase >= 2u)
            ? (uint8_t)(fatigue_gain + 3u)
            : (uint8_t)(fatigue_gain + 6u);
    }
    if((race_tick & 0x0Fu) == 0u && state->fatigue > 0u) {
        state->fatigue--;
    }
    state->fatigue = (uint8_t)(state->fatigue + fatigue_gain);

    return distance;
}