#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stdint.h>

#define TEXT_STRIP_CACHE_CHARS 33

struct TextStripBlock {
    uint8_t base_tile;
    uint8_t span;
    uint8_t map_x;
    uint8_t map_y;
    uint8_t fg;
    uint8_t bg;
    uint8_t pixel_y;
    uint8_t opaque;
    uint8_t has_rendered;
    uint8_t cached_fg;
    uint8_t cached_bg;
    uint8_t cached_pixel_y;
    uint8_t cached_opaque;
    char cached_text[TEXT_STRIP_CACHE_CHARS];
};

struct TextPairBlock {
    uint8_t base_tile;
    uint8_t span;
    uint8_t map_x;
    uint8_t map_y;
    uint8_t fg;
    uint8_t bg;
};

uint8_t text_measure_span(const char *text);
void text_reset_arena(uint8_t base_tile, uint8_t limit_tile);
uint8_t text_alloc_tiles(uint8_t span);

void text_draw_strip_alloc(uint8_t map_x, uint8_t map_y,
    const char *text, uint8_t fg, uint8_t bg, uint8_t pixel_y, bool opaque);
void text_draw_pair_alloc(uint8_t map_x, uint8_t map_y,
    const char *top_text, const char *bottom_text, uint8_t fg, uint8_t bg);

void text_init_strip_block(struct TextStripBlock *block,
    uint8_t map_x, uint8_t map_y, uint8_t span,
    uint8_t fg, uint8_t bg, uint8_t pixel_y, bool opaque);
void text_update_strip_block(struct TextStripBlock *block, const char *text);

void text_init_pair_block(struct TextPairBlock *block,
    uint8_t map_x, uint8_t map_y, uint8_t span, uint8_t fg, uint8_t bg);
void text_update_pair_block(const struct TextPairBlock *block,
    const char *top_text, const char *bottom_text);

#endif