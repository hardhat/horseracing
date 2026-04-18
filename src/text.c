#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "main.h"
#include "text.h"

static uint8_t text_tile_next;
static uint8_t text_tile_limit;

static uint8_t max_u8(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

uint8_t text_measure_span(const char *text)
{
    size_t len = strlen(text);
    return (uint8_t)((len + 1u) >> 1);
}

void text_reset_arena(uint8_t base_tile, uint8_t limit_tile)
{
    text_tile_next = base_tile;
    text_tile_limit = limit_tile;
}

uint8_t text_alloc_tiles(uint8_t span)
{
    uint8_t base = text_tile_next;

    if(span == 0u) {
        return base;
    }

    if((uint16_t)base + span > (uint16_t)text_tile_limit) {
        debug_log("text: tile arena exhausted");
        return text_tile_next;
    }

    text_tile_next = (uint8_t)(text_tile_next + span);
    return base;
}

void text_draw_strip_alloc(uint8_t map_x, uint8_t map_y,
    const char *text, uint8_t fg, uint8_t bg, uint8_t pixel_y, bool opaque)
{
    struct TextStripBlock block;

    text_init_strip_block(&block, map_x, map_y, text_measure_span(text), fg, bg, pixel_y, opaque);
    text_update_strip_block(&block, text);
}

void text_draw_pair_alloc(uint8_t map_x, uint8_t map_y,
    const char *top_text, const char *bottom_text, uint8_t fg, uint8_t bg)
{
    struct TextPairBlock block;

    text_init_pair_block(&block, map_x, map_y,
        max_u8(text_measure_span(top_text), text_measure_span(bottom_text)), fg, bg);
    text_update_pair_block(&block, top_text, bottom_text);
}

void text_init_strip_block(struct TextStripBlock *block,
    uint8_t map_x, uint8_t map_y, uint8_t span,
    uint8_t fg, uint8_t bg, uint8_t pixel_y, bool opaque)
{
    block->base_tile = text_alloc_tiles(span);
    block->span = span;
    block->map_x = map_x;
    block->map_y = map_y;
    block->fg = fg;
    block->bg = bg;
    block->pixel_y = pixel_y;
    block->opaque = opaque ? 1u : 0u;

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(block->base_tile + i));
    }
}

void text_update_strip_block(const struct TextStripBlock *block, const char *text)
{
    clear_text_tiles(block->bg, block->span);
    if(block->opaque) {
        draw_text_opaque(0, block->pixel_y, text, block->fg, block->bg);
    } else {
        draw_text(0, block->pixel_y, text, block->fg);
    }
    render_text(block->base_tile, block->span);
}

void text_init_pair_block(struct TextPairBlock *block,
    uint8_t map_x, uint8_t map_y, uint8_t span, uint8_t fg, uint8_t bg)
{
    block->base_tile = text_alloc_tiles(span);
    block->span = span;
    block->map_x = map_x;
    block->map_y = map_y;
    block->fg = fg;
    block->bg = bg;

    for(uint8_t i = 0; i < span; i++) {
        draw_tilemap((uint16_t)(map_x + i), map_y, (uint8_t)(block->base_tile + i));
    }
}

void text_update_pair_block(const struct TextPairBlock *block,
    const char *top_text, const char *bottom_text)
{
    clear_text_tiles(block->bg, block->span);
    draw_text(0, 0, top_text, block->fg);
    draw_text(0, 8, bottom_text, block->fg);
    render_text(block->base_tile, block->span);
}