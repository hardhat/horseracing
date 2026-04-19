#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "race_sim.h"

enum {
    SURFACE_DRY = 0u,
    SURFACE_MUDDY = 1u,
    SURFACE_TURF = 2u,
};

static struct RaceSimTraits make_traits(uint8_t break_speed, uint8_t early_pace,
    uint8_t endurance, uint8_t stretch_speed,
    uint8_t dry_affinity, uint8_t muddy_affinity, uint8_t turf_affinity)
{
    struct RaceSimTraits traits;

    traits.break_speed = break_speed;
    traits.early_pace = early_pace;
    traits.endurance = endurance;
    traits.stretch_speed = stretch_speed;
    traits.surface_affinity[SURFACE_DRY] = dry_affinity;
    traits.surface_affinity[SURFACE_MUDDY] = muddy_affinity;
    traits.surface_affinity[SURFACE_TURF] = turf_affinity;

    return traits;
}

static uint16_t finish_tick_for_horse(const struct RaceSimTraits *traits,
    uint8_t surface, uint8_t distance_furlongs)
{
    struct RaceSimState state;
    uint16_t total_units = race_sim_total_distance_units(distance_furlongs);
    uint16_t tick = 0u;

    race_sim_reset_state(&state);

    while(state.distance_units < total_units) {
        uint8_t step = race_sim_step_distance(traits, surface, distance_furlongs,
            &state, true, false, false, tick, 0u);
        race_sim_advance_state(&state, step, total_units);
        tick++;
        assert(tick < 512u);
    }

    return tick;
}

static struct RaceSimState simulate_steps(const struct RaceSimTraits *traits,
    uint8_t surface, uint8_t distance_furlongs,
    uint8_t steps, bool coax, bool whip)
{
    struct RaceSimState state;
    uint16_t total_units = race_sim_total_distance_units(distance_furlongs);

    race_sim_reset_state(&state);

    for(uint8_t tick = 0u; tick < steps; tick++) {
        uint8_t step = race_sim_step_distance(traits, surface, distance_furlongs,
            &state, false, coax, whip, tick, 0u);
        race_sim_advance_state(&state, step, total_units);
    }

    return state;
}

static void test_short_race_favors_sprinter(void)
{
    struct RaceSimTraits sprinter = make_traits(12u, 12u, 4u, 4u, 8u, 8u, 8u);
    struct RaceSimTraits stayer = make_traits(4u, 4u, 12u, 12u, 8u, 8u, 8u);

    assert(finish_tick_for_horse(&sprinter, SURFACE_DRY, 4u)
        < finish_tick_for_horse(&stayer, SURFACE_DRY, 4u));
}

static void test_long_race_favors_stayer(void)
{
    struct RaceSimTraits sprinter = make_traits(12u, 12u, 4u, 4u, 8u, 8u, 8u);
    struct RaceSimTraits stayer = make_traits(4u, 4u, 12u, 12u, 8u, 8u, 8u);

    assert(finish_tick_for_horse(&stayer, SURFACE_DRY, 10u)
        < finish_tick_for_horse(&sprinter, SURFACE_DRY, 10u));
}

static void test_muddy_surface_increases_fatigue(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 9u, 9u, 9u, 9u, 9u, 9u);
    struct RaceSimState dry_state = simulate_steps(&balanced, SURFACE_DRY, 8u, 10u, false, false);
    struct RaceSimState muddy_state = simulate_steps(&balanced, SURFACE_MUDDY, 8u, 10u, false, false);

    assert(muddy_state.fatigue > dry_state.fatigue);
}

static void test_surface_affinity_changes_outcome(void)
{
    struct RaceSimTraits turf_specialist = make_traits(9u, 9u, 9u, 10u, 6u, 6u, 12u);

    assert(finish_tick_for_horse(&turf_specialist, SURFACE_TURF, 8u)
        < finish_tick_for_horse(&turf_specialist, SURFACE_DRY, 8u));
    assert(finish_tick_for_horse(&turf_specialist, SURFACE_TURF, 8u)
        < finish_tick_for_horse(&turf_specialist, SURFACE_MUDDY, 8u));
}

static void test_coax_is_modest_and_costly(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 9u, 9u, 9u, 8u, 8u, 8u);
    struct RaceSimState steady_state = simulate_steps(&balanced, SURFACE_DRY, 8u, 8u, false, false);
    struct RaceSimState coax_state = simulate_steps(&balanced, SURFACE_DRY, 8u, 8u, true, false);

    assert(coax_state.distance_units > steady_state.distance_units);
    assert(coax_state.fatigue > steady_state.fatigue);
}

static void test_late_whip_beats_early_whip(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 9u, 9u, 9u, 8u, 8u, 8u);
    struct RaceSimState early_state;
    struct RaceSimState late_state;
    uint16_t total_units = race_sim_total_distance_units(10u);
    uint8_t early_step;
    uint8_t late_step;

    race_sim_reset_state(&early_state);
    race_sim_reset_state(&late_state);
    late_state.distance_units = (uint16_t)((total_units * 3u) / 4u);

    early_step = race_sim_step_distance(&balanced, SURFACE_DRY, 10u,
        &early_state, false, false, true, 0u, 0u);
    late_step = race_sim_step_distance(&balanced, SURFACE_DRY, 10u,
        &late_state, false, false, true, 0u, 0u);

    assert(late_step >= early_step);
    assert(late_state.fatigue < early_state.fatigue);
}

static void test_cpu_saves_whip_for_late_race(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 9u, 10u, 10u, 8u, 8u, 8u);
    struct RaceSimState early_state;
    struct RaceSimState late_state;
    uint16_t total_units = race_sim_total_distance_units(10u);

    race_sim_reset_state(&early_state);
    race_sim_reset_state(&late_state);
    early_state.distance_units = (uint16_t)(total_units / 4u);
    late_state.distance_units = (uint16_t)((total_units * 3u) / 4u);

    (void)race_sim_step_distance(&balanced, SURFACE_DRY, 10u,
        &early_state, true, false, false, 0u, 0u);
    (void)race_sim_step_distance(&balanced, SURFACE_DRY, 10u,
        &late_state, true, false, false, 0u, 0u);

    assert(early_state.cpu_whip_uses == 0u);
    assert(early_state.whip_boost == 0u);
    assert(late_state.cpu_whip_uses == 1u);
    assert(late_state.whip_boost > 0u);
}

static void test_cpu_whip_has_limited_bursts(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 9u, 10u, 10u, 8u, 8u, 8u);
    struct RaceSimState state;
    uint16_t total_units = race_sim_total_distance_units(10u);

    race_sim_reset_state(&state);
    state.distance_units = (uint16_t)((total_units * 3u) / 4u);

    for(uint16_t tick = 0u; tick < 24u; tick++) {
        uint8_t step = race_sim_step_distance(&balanced, SURFACE_DRY, 10u,
            &state, true, false, false, tick, 0u);
        race_sim_advance_state(&state, step, total_units);
    }

    assert(state.cpu_whip_uses <= 2u);
}

static void test_deterministic_results(void)
{
    struct RaceSimTraits balanced = make_traits(9u, 8u, 10u, 9u, 8u, 7u, 9u);
    uint16_t run_a = finish_tick_for_horse(&balanced, SURFACE_TURF, 6u);
    uint16_t run_b = finish_tick_for_horse(&balanced, SURFACE_TURF, 6u);

    assert(run_a == run_b);
}

int main(void)
{
    test_short_race_favors_sprinter();
    test_long_race_favors_stayer();
    test_muddy_surface_increases_fatigue();
    test_surface_affinity_changes_outcome();
    test_coax_is_modest_and_costly();
    test_late_whip_beats_early_whip();
    test_cpu_saves_whip_for_late_race();
    test_cpu_whip_has_limited_bursts();
    test_deterministic_results();

    puts("race_sim tests passed");
    return 0;
}