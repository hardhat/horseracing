#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "game_race.h"
#include "game_results.h"
#include "main.h"
#include "race_sim.h"
#include "session_logic.h"
#include "text.h"

#define TEXT_TILE_ARENA_BASE  ((uint8_t)0x9Cu)
#define TEXT_TILE_ARENA_LIMIT ((uint8_t)0xFFu)
#define RACE_RESULT_DELAY ((uint16_t)150u)
#define RACE_UNIT_PIXELS ((uint16_t)6u)
#define RACE_WORLD_MARGIN ((uint16_t)32u)
#define RACE_CAMERA_TARGET_X ((uint16_t)120u)
#define RACE_LANE_NEAR_PIXELS ((uint16_t)20u)
#define RACE_LANE_AHEAD_PIXELS ((uint16_t)36u)
#define RACE_LANE_SWITCH_DELAY ((uint8_t)8u)
#define RACE_FURLONG_UNITS ((uint16_t)8u)

static bool race_held[MAX_INPUT];
static struct RaceSimState race_state[4];
static uint8_t race_finish_order[4];
static uint8_t race_finish_count;
static uint8_t race_controlled_entry;
static uint16_t race_tick;
static uint8_t race_show_results;
static uint8_t race_show_standings;
static uint16_t race_result_timer;
static uint8_t race_lane_cooldown[4];
static uint16_t race_camera_scroll_x;
static uint8_t race_furlongs_remaining;
static struct TextStripBlock race_furlong_block;

static const char *const surface_name[SURFACE_COUNT] = {
    "DRY",
    "MUD",
    "TURF",
};

static const uint8_t race_lane_y[4] = { 4u, 6u, 8u, 10u };

static uint16_t race_total_units(void)
{
    return race_sim_total_distance_units(
        game.session.races[game.session.current_race].distance_furlongs);
}

static uint16_t race_entry_world_x(uint8_t entry_index)
{
    return (uint16_t)(RACE_WORLD_MARGIN + race_state[entry_index].distance_units * RACE_UNIT_PIXELS);
}

static uint16_t race_world_length_pixels(void)
{
    return (uint16_t)(race_total_units() * RACE_UNIT_PIXELS + RACE_WORLD_MARGIN + 32u);
}

static uint8_t leader_race_entry(void)
{
    uint8_t leader = 0u;

    for(uint8_t i = 1u; i < 4u; i++) {
        if(race_state[i].distance_units > race_state[leader].distance_units) {
            leader = i;
        }
    }

    return leader;
}

static bool lane_has_room(uint8_t entry_index, uint8_t lane)
{
    uint16_t own_x = race_entry_world_x(entry_index);

    for(uint8_t other = 0u; other < 4u; other++) {
        uint16_t other_x;

        if(other == entry_index) {
            continue;
        }
        if(game.session.races[game.session.current_race].entries[other].lane != lane) {
            continue;
        }

        other_x = race_entry_world_x(other);
        if(other_x > own_x) {
            if((uint16_t)(other_x - own_x) < RACE_LANE_NEAR_PIXELS) {
                return false;
            }
        } else if((uint16_t)(own_x - other_x) < RACE_LANE_NEAR_PIXELS) {
            return false;
        }
    }

    return true;
}

static bool lane_is_blocked_ahead(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;
    uint16_t own_x = race_entry_world_x(entry_index);

    for(uint8_t other = 0u; other < 4u; other++) {
        uint16_t other_x;

        if(other == entry_index) {
            continue;
        }
        if(game.session.races[game.session.current_race].entries[other].lane != lane) {
            continue;
        }

        other_x = race_entry_world_x(other);
        if(other_x > own_x && (uint16_t)(other_x - own_x) <= RACE_LANE_AHEAD_PIXELS) {
            return true;
        }
    }

    return false;
}

static void move_race_entry_lane(uint8_t entry_index, uint8_t next_lane)
{
    if(next_lane >= 4u) {
        return;
    }

    game.session.races[game.session.current_race].entries[entry_index].lane = next_lane;
    race_lane_cooldown[entry_index] = RACE_LANE_SWITCH_DELAY;
}

static void update_race_camera(void)
{
    uint16_t leader_x = race_entry_world_x(leader_race_entry());
    uint16_t max_scroll = race_world_length_pixels();

    if(max_scroll > 320u) {
        max_scroll = (uint16_t)(max_scroll - 320u);
    } else {
        max_scroll = 0u;
    }

    if(leader_x > RACE_CAMERA_TARGET_X) {
        race_camera_scroll_x = (uint16_t)(leader_x - RACE_CAMERA_TARGET_X);
        if(race_camera_scroll_x > max_scroll) {
            race_camera_scroll_x = max_scroll;
        }
    } else {
        race_camera_scroll_x = 0u;
    }

    set_layer_scroll(0u, 0u, 0u);
    set_layer_scroll(1u, (uint16_t)(race_camera_scroll_x >> 2), 0u);
}

static void update_race_furlong_status(void)
{
    uint16_t total_units = race_total_units();
    uint16_t leader_units = race_state[leader_race_entry()].distance_units;
    uint8_t remaining = 0u;
    char line[8];

    if(total_units > leader_units) {
        remaining = (uint8_t)((total_units - leader_units + (RACE_FURLONG_UNITS - 1u)) / RACE_FURLONG_UNITS);
    }

    if(remaining == race_furlongs_remaining) {
        return;
    }

    race_furlongs_remaining = remaining;
    if(remaining == 0u) {
        strcpy(line, "HOME");
    } else {
        sprintf(line, "%uF GO", (unsigned)remaining);
    }
    text_update_strip_block(&race_furlong_block, line);
    render_tilemap(0);
}

static uint8_t horse_icon_tile(uint8_t color)
{
    switch(color) {
        case HORSE_COLOR_WHITE:
            return TILE_HUD_HORSE_WHITE;
        case HORSE_COLOR_GREEN:
            return TILE_HUD_HORSE_GREEN;
        case HORSE_COLOR_GOLD:
            return TILE_HUD_HORSE_GOLD;
        default:
            return TILE_HUD_HORSE_RED;
    }
}

static uint8_t entry_marker_tile(uint8_t entry_index)
{
    switch(entry_index) {
        case 0:
            return TILE_HUD_MARKER_FIRST;
        case 1:
            return TILE_HUD_MARKER_SECOND;
        case 2:
            return TILE_HUD_PP_TIME;
        default:
            return TILE_HUD_PP_FINISH;
    }
}

static uint8_t surface_track_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_CLEAR_0;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_CLEAR_0;

        default:
            return TILE_TRACK_DRY_CLEAR_0;
    }
}

static uint8_t surface_backdrop_tile(uint8_t surface, uint8_t column)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return (column & 1u) == 0u ? TILE_TRACK_MUD_GRANDSTAND_0 : TILE_TRACK_MUD_CROWD_0;

        case SURFACE_TURF:
            return (column & 1u) == 0u ? TILE_TRACK_TURF_GRANDSTAND_0 : TILE_TRACK_TURF_CROWD_0;

        default:
            return (column & 1u) == 0u ? TILE_TRACK_DRY_GRANDSTAND_0 : TILE_TRACK_DRY_CROWD_0;
    }
}

static uint8_t surface_furlong_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_FURLONG;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_FURLONG;

        default:
            return TILE_TRACK_DRY_FURLONG;
    }
}

static uint8_t surface_finish_tile(uint8_t surface)
{
    switch(surface) {
        case SURFACE_MUDDY:
            return TILE_TRACK_MUD_FINISH;

        case SURFACE_TURF:
            return TILE_TRACK_TURF_FINISH;

        default:
            return TILE_TRACK_DRY_FINISH;
    }
}

static uint8_t controlled_race_entry(void)
{
    switch(game.session.jockey_assignment) {
        case JOCKEY_ASSIGN_OUTSIDE_1:
            return 0u;

        case JOCKEY_ASSIGN_OUTSIDE_4:
            return 3u;

        default:
            return 0xFFu;
    }
}

static void record_race_result(void)
{
    uint8_t finish_order[4];

    for(uint8_t i = 0; i < 4u; i++) {
        finish_order[i] = race_finish_order[i];
    }

    session_logic_record_race_result(&game.session,
        game.session.current_race,
        finish_order,
        (uint16_t)(600u + race_tick * 3u));
}

static void finish_race_and_show_results(void)
{
    if(race_show_results != 0u) {
        return;
    }

    record_race_result();
    session_logic_settle_race_wagers(&game.session);
    race_show_results = 1u;
    race_result_timer = RACE_RESULT_DELAY;
    clear_sprites();
    game_results_render_race_results_screen();
}

static void advance_after_results(void)
{
    if(session_logic_advance_after_results(&game.session)) {
        game.resume_session = 1u;
        set_state(GAME_STATE_GAME);
        return;
    }

    race_show_standings = 1u;
    race_show_results = 0u;
    clear_sprites();
    game_results_render_final_standings_screen();
}

static void build_race_backdrop_layer(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    for(uint8_t x = 0u; x < TILES_ACROSS; x++) {
        draw_tilemap(x, 2u, surface_backdrop_tile(race->surface, x));
        draw_tilemap(x, 3u, surface_backdrop_tile(race->surface, (uint8_t)(x + 1u)));
    }
    render_tilemap(1);
}

static void build_race_foreground_layer(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    uint8_t track_tile = surface_track_tile(race->surface);

    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    fill_tilemap(TILE_TRACK_UTIL_DRY_0, 0, 2u, TILES_ACROSS, 1u);
    fill_tilemap(TILE_TRACK_UTIL_RAIL_CURVE_0, 0, 12u, TILES_ACROSS, 1u);

    for(uint8_t lane = 0u; lane < 4u; lane++) {
        uint8_t y = race_lane_y[lane];

        fill_tilemap(track_tile, 0, y, TILES_ACROSS, 1u);
        draw_tilemap(0, y, entry_marker_tile(lane));
        draw_tilemap(19, y, surface_finish_tile(race->surface));
    }

    render_tilemap(0);
}

static void draw_race_world_sprites(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    uint16_t total_units = race_total_units();
    uint16_t finish_x = (uint16_t)(RACE_WORLD_MARGIN + total_units * RACE_UNIT_PIXELS);

    reset_sprite();

    for(uint8_t marker = 1u; marker < race->distance_furlongs; marker++) {
        uint16_t marker_units = (uint16_t)(marker * RACE_FURLONG_UNITS);
        uint16_t marker_x = (uint16_t)(RACE_WORLD_MARGIN + marker_units * RACE_UNIT_PIXELS);

        if(marker_x + 16u >= race_camera_scroll_x && marker_x < race_camera_scroll_x + 304u) {
            add_sprite((uint16_t)(marker_x - race_camera_scroll_x), 40u, surface_furlong_tile(race->surface));
        }
    }

    if(finish_x + 16u >= race_camera_scroll_x && finish_x < race_camera_scroll_x + 304u) {
        add_sprite((uint16_t)(finish_x - race_camera_scroll_x), 40u,
            surface_finish_tile(race->surface));
    }

    for(uint8_t i = 0u; i < 4u; i++) {
        uint16_t horse_x = race_entry_world_x(i);
        uint8_t lane = race->entries[i].lane;
        struct Horse *horse = &game.session.stable[race->entries[i].horse_index];

        if(horse_x + 16u < race_camera_scroll_x || horse_x > race_camera_scroll_x + 304u) {
            continue;
        }

        add_sprite((uint16_t)(horse_x - race_camera_scroll_x),
            (uint8_t)(race_lane_y[lane] * 16u),
            horse_icon_tile(horse->color));
    }

    render_sprites();
}

static void advance_race_entry(uint8_t entry_index, uint8_t distance)
{
    uint16_t total_units = race_sim_total_distance_units(
        game.session.races[game.session.current_race].distance_furlongs);

    if(race_state[entry_index].distance_units >= total_units) {
        return;
    }

    race_sim_advance_state(&race_state[entry_index], distance, total_units);

    if(race_state[entry_index].distance_units >= total_units) {
        race_finish_order[race_finish_count] = entry_index;
        race_finish_count++;
    }
}

static void update_controlled_lane(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;

    if(race_lane_cooldown[entry_index] != 0u) {
        race_lane_cooldown[entry_index]--;
        return;
    }

    if(race_held[INPUT_UP] && lane > 0u && lane_has_room(entry_index, (uint8_t)(lane - 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane - 1u));
        return;
    }

    if(race_held[INPUT_DOWN] && lane + 1u < 4u && lane_has_room(entry_index, (uint8_t)(lane + 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane + 1u));
    }
}

static void update_cpu_lane(uint8_t entry_index)
{
    uint8_t lane = game.session.races[game.session.current_race].entries[entry_index].lane;
    bool blocked;

    if(race_lane_cooldown[entry_index] != 0u) {
        race_lane_cooldown[entry_index]--;
        return;
    }

    blocked = lane_is_blocked_ahead(entry_index);
    if(blocked && lane + 1u < 4u && lane_has_room(entry_index, (uint8_t)(lane + 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane + 1u));
        return;
    }

    if(lane > 0u && lane_has_room(entry_index, (uint8_t)(lane - 1u))) {
        move_race_entry_lane(entry_index, (uint8_t)(lane - 1u));
    }
}

static void update_race_lanes(void)
{
    for(uint8_t i = 0u; i < 4u; i++) {
        if(i == race_controlled_entry) {
            update_controlled_lane(i);
        } else {
            update_cpu_lane(i);
        }
    }
}

static uint8_t race_step_distance(uint8_t entry_index)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    struct Horse *horse = &game.session.stable[race->entries[entry_index].horse_index];
    struct RaceSimTraits traits;
    uint8_t distance;
    uint8_t lane = race->entries[entry_index].lane;

    traits.break_speed = horse->traits.break_speed;
    traits.early_pace = horse->traits.early_pace;
    traits.endurance = horse->traits.endurance;
    traits.stretch_speed = horse->traits.stretch_speed;
    for(uint8_t surface = 0u; surface < SURFACE_COUNT; surface++) {
        traits.surface_affinity[surface] = horse->traits.surface_affinity[surface];
    }

    distance = race_sim_step_distance(&traits,
        race->surface,
        race->distance_furlongs,
        &race_state[entry_index],
        entry_index != race_controlled_entry,
        entry_index == race_controlled_entry && race_held[INPUT_Y],
        entry_index == race_controlled_entry && race_held[INPUT_B],
        race_tick,
        game_next_random_u16());

    if(lane == 0u && (race_tick & 0x0Fu) == 0u) {
        distance++;
    } else if(lane == 1u && (race_tick & 0x1Fu) == 0u) {
        distance++;
    } else if(lane == 3u && distance > 1u && (race_tick & 0x1Fu) == 0u) {
        distance--;
    }

    return distance;
}

void game_race_init(void)
{
    struct RaceCard *race = &game.session.races[game.session.current_race];
    char header_line[24];

    reset_layer_scrolls();
    fill_tilemap(TILE_COLOR_BLACK, 0, 0, TILES_ACROSS, TILES_DOWN);
    render_tilemap(1);
    render_tilemap(0);

    text_reset_arena(TEXT_TILE_ARENA_BASE, TEXT_TILE_ARENA_LIMIT);
    memset(race_held, 0, sizeof(race_held));
    for(uint8_t i = 0u; i < 4u; i++) {
        race_sim_reset_state(&race_state[i]);
    }
    memset(race_finish_order, 0xFF, sizeof(race_finish_order));
    race_finish_count = 0u;
    race_controlled_entry = controlled_race_entry();
    race_tick = 0u;
    race_show_results = 0u;
    race_show_standings = 0u;
    race_result_timer = 0u;
    race_camera_scroll_x = 0u;
    race_furlongs_remaining = 0xFFu;
    memset(race_lane_cooldown, 0, sizeof(race_lane_cooldown));

    for(uint8_t i = 0u; i < 4u; i++) {
        race->entries[i].lane = i;
    }

    build_race_backdrop_layer();
    build_race_foreground_layer();

    sprintf(header_line, "R%u %s %uF",
        (unsigned)(game.session.current_race + 1u),
        surface_name[race->surface],
        (unsigned)race->distance_furlongs);

    set_font(FONT_NICETY_BOLD);
    text_draw_strip_alloc(2, 0, "RACE PROTOTYPE", COL_WHITE, COL_BLACK, 4, false);
    set_font(FONT_NICETY);
    text_draw_strip_alloc(5, 1, header_line, COL_LIGHT_GRAY, COL_BLACK, 4, false);
    text_init_strip_block(&race_furlong_block, 13, 1, 4u, COL_YELLOW, COL_BLACK, 4u, false);
    if(race_controlled_entry == 0xFFu) {
        text_draw_pair_alloc(3, 13,
            "CPU RACE",
            "START EXIT",
            COL_LIGHT_GRAY, COL_BLACK);
    } else {
        text_draw_pair_alloc(2, 13,
            "U/D LANE Y/B GO",
            "START EXIT",
            COL_LIGHT_GRAY, COL_BLACK);
    }

    render_tilemap(0);
    update_race_camera();
    update_race_furlong_status();
    clear_sprites();
    debug_log("game: race prototype screen");
}

void game_race_input(uint8_t key, bool down)
{
    if(race_show_standings != 0u) {
        if(down && (key == INPUT_START || key == INPUT_A)) {
            set_state(GAME_STATE_MENU);
        }
        return;
    }

    if(race_show_results != 0u) {
        return;
    }

    if(key < MAX_INPUT) {
        race_held[key] = down;
    }

    if(down && key == INPUT_START) {
        set_state(GAME_STATE_MENU);
    }
}

void game_race_update(void)
{
    if(race_show_standings != 0u) {
        return;
    }

    if(race_show_results != 0u) {
        if(race_result_timer > 0u) {
            race_result_timer--;
        }

        if(race_result_timer == 0u) {
            advance_after_results();
        }
        return;
    }

    race_tick++;
    update_race_lanes();
    update_race_camera();
    update_race_furlong_status();

    if((race_tick & 0x07u) != 0u || race_finish_count >= 4u) {
        if(race_finish_count >= 4u) {
            finish_race_and_show_results();
        }
        return;
    }

    for(uint8_t i = 0; i < 4u; i++) {
        advance_race_entry(i, race_step_distance(i));
    }
    update_race_camera();
    update_race_furlong_status();

    if(race_finish_count >= 4u) {
        finish_race_and_show_results();
    }
}

void game_race_draw(void)
{
    if(race_show_results == 0u && race_show_standings == 0u) {
        draw_race_world_sprites();
    }
}