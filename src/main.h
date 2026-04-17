#ifndef MAIN_H
#define MAIN_H

#include<stdint.h>

enum GAME_STATE
{
    GAME_STATE_MENU,
    GAME_STATE_WORLDMAP,
    GAME_STATE_GAME,
    GAME_STATE_CHOICE,
    GAME_STATE_BATTLE,
    GAME_STATE_GAMEOVER
};

enum INPUT
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_X,
    INPUT_Y,
    INPUT_START,
    INPUT_SELECT,
    INPUT_L,
    INPUT_R,
    MAX_INPUT
};

enum {
    COL_BLACK         = 0x0,
    COL_DARK_BLUE     = 0x1,
    COL_DARK_GREEN    = 0x2,
    COL_DARK_CYAN     = 0x3,
    COL_DARK_RED      = 0x4,
    COL_DARK_MAGENTA  = 0x5,
    COL_BROWN         = 0x6,
    COL_LIGHT_GRAY    = 0x7,
    COL_DARK_GRAY     = 0x8,
    COL_BLUE          = 0x9,
    COL_GREEN         = 0xa,
    COL_CYAN          = 0xb,
    COL_RED           = 0xc,
    COL_MAGENTA       = 0xd,
    COL_YELLOW        = 0xe,
    COL_WHITE         = 0xf,
};

enum TILE_COLORS
{
    TILE_COLOR_BLACK = 0,
    TILE_COLOR_WHITE = 1, // 1-4 are white
    TILE_COLOR_RED = 5, // 5-8 are red
    TILE_COLOR_GREEN = 9, // 9-12 are green
    TILE_COLOR_BLUE = 13,
    TILE_CURSOR = 14,
    TILE_COLOR_TITLE   = 18,   // title tileset:   floor(285/16)=17 × 2 = 34 tiles (18-51)
    TILE_COLOR_MAP     = 52,   // map tileset:     16 × 4 = 64 tiles (52-115)
    TILE_COLOR_HUD     = 116,  // hud tileset:     8 × 3 = 24 tiles (116-139)
    TILE_COLOR_SPRITES = 140,  // sprite tileset:  8 × 2 = 16 tiles (140-155)

    TILE_SPRITE_PLACEHOLDER_0_TOP    = TILE_COLOR_SPRITES + 0,
    TILE_SPRITE_PLACEHOLDER_1_TOP    = TILE_COLOR_SPRITES + 1,
    TILE_SPRITE_PLACEHOLDER_0_BOTTOM = TILE_COLOR_SPRITES + 8,
    TILE_SPRITE_PLACEHOLDER_1_BOTTOM = TILE_COLOR_SPRITES + 9,

    TILE_HUD_PANEL_FRAME_0           = TILE_COLOR_HUD + 0,
    TILE_HUD_PANEL_FRAME_1           = TILE_COLOR_HUD + 1,
    TILE_HUD_PANEL_FRAME_2           = TILE_COLOR_HUD + 2,
    TILE_HUD_PANEL_FRAME_3           = TILE_COLOR_HUD + 3,
    TILE_HUD_LABEL_RACE              = TILE_COLOR_HUD + 4,
    TILE_HUD_LABEL_BANK              = TILE_COLOR_HUD + 5,
    TILE_HUD_LABEL_ODDS              = TILE_COLOR_HUD + 6,
    TILE_HUD_LABEL_POST              = TILE_COLOR_HUD + 7,
    TILE_HUD_LABEL_WIN               = TILE_COLOR_HUD + 8,
    TILE_HUD_LABEL_EXACTA            = TILE_COLOR_HUD + 9,
    TILE_HUD_LABEL_MUD               = TILE_COLOR_HUD + 10,
    TILE_HUD_LABEL_TURF              = TILE_COLOR_HUD + 11,
    TILE_HUD_BADGE_RACE              = TILE_COLOR_HUD + 12,
    TILE_HUD_BADGE_BANKROLL          = TILE_COLOR_HUD + 13,
    TILE_HUD_EXACTA_ARROW            = TILE_COLOR_HUD + 14,
    TILE_HUD_SELECTOR_BOX            = TILE_COLOR_HUD + 15,
    TILE_HUD_HORSE_WHITE             = TILE_COLOR_HUD + 16,
    TILE_HUD_HORSE_GREEN             = TILE_COLOR_HUD + 17,
    TILE_HUD_HORSE_GOLD              = TILE_COLOR_HUD + 18,
    TILE_HUD_HORSE_RED               = TILE_COLOR_HUD + 19,
    TILE_HUD_MARKER_FIRST            = TILE_COLOR_HUD + 20,
    TILE_HUD_MARKER_SECOND           = TILE_COLOR_HUD + 21,
    TILE_HUD_PP_TIME                 = TILE_COLOR_HUD + 22,
    TILE_HUD_PP_FINISH               = TILE_COLOR_HUD + 23,

    // Prototype horse-racing track sheet aliases matching img/map.gif.
    TILE_TRACK_DRY_CLEAR_0           = TILE_COLOR_MAP + 0,
    TILE_TRACK_DRY_CLEAR_1           = TILE_COLOR_MAP + 1,
    TILE_TRACK_DRY_CLEAR_2           = TILE_COLOR_MAP + 2,
    TILE_TRACK_DRY_CLEAR_3           = TILE_COLOR_MAP + 3,
    TILE_TRACK_DRY_CROWD_0           = TILE_COLOR_MAP + 4,
    TILE_TRACK_DRY_CROWD_1           = TILE_COLOR_MAP + 5,
    TILE_TRACK_DRY_FINISH            = TILE_COLOR_MAP + 6,
    TILE_TRACK_DRY_FURLONG           = TILE_COLOR_MAP + 7,
    TILE_TRACK_DRY_GRANDSTAND_0      = TILE_COLOR_MAP + 8,
    TILE_TRACK_DRY_GRANDSTAND_1      = TILE_COLOR_MAP + 9,
    TILE_TRACK_DRY_GRANDSTAND_RUN    = TILE_COLOR_MAP + 10,
    TILE_TRACK_DRY_FINISH_ALT        = TILE_COLOR_MAP + 11,
    TILE_TRACK_DRY_FURLONG_ALT       = TILE_COLOR_MAP + 12,
    TILE_TRACK_DRY_HORSE_LIGHT       = TILE_COLOR_MAP + 13,
    TILE_TRACK_DRY_HORSE_GREEN       = TILE_COLOR_MAP + 14,
    TILE_TRACK_DRY_MARKER            = TILE_COLOR_MAP + 15,

    TILE_TRACK_MUD_CLEAR_0           = TILE_COLOR_MAP + 16,
    TILE_TRACK_MUD_CLEAR_1           = TILE_COLOR_MAP + 17,
    TILE_TRACK_MUD_CLEAR_2           = TILE_COLOR_MAP + 18,
    TILE_TRACK_MUD_CLEAR_3           = TILE_COLOR_MAP + 19,
    TILE_TRACK_MUD_CROWD_0           = TILE_COLOR_MAP + 20,
    TILE_TRACK_MUD_CROWD_1           = TILE_COLOR_MAP + 21,
    TILE_TRACK_MUD_FINISH            = TILE_COLOR_MAP + 22,
    TILE_TRACK_MUD_FURLONG           = TILE_COLOR_MAP + 23,
    TILE_TRACK_MUD_GRANDSTAND_0      = TILE_COLOR_MAP + 24,
    TILE_TRACK_MUD_GRANDSTAND_1      = TILE_COLOR_MAP + 25,
    TILE_TRACK_MUD_GRANDSTAND_RUN    = TILE_COLOR_MAP + 26,
    TILE_TRACK_MUD_FINISH_ALT        = TILE_COLOR_MAP + 27,
    TILE_TRACK_MUD_FURLONG_ALT       = TILE_COLOR_MAP + 28,
    TILE_TRACK_MUD_HORSE_LIGHT       = TILE_COLOR_MAP + 29,
    TILE_TRACK_MUD_HORSE_RED         = TILE_COLOR_MAP + 30,
    TILE_TRACK_MUD_MARKER            = TILE_COLOR_MAP + 31,

    TILE_TRACK_TURF_CLEAR_0          = TILE_COLOR_MAP + 32,
    TILE_TRACK_TURF_CLEAR_1          = TILE_COLOR_MAP + 33,
    TILE_TRACK_TURF_CLEAR_2          = TILE_COLOR_MAP + 34,
    TILE_TRACK_TURF_CLEAR_3          = TILE_COLOR_MAP + 35,
    TILE_TRACK_TURF_CROWD_0          = TILE_COLOR_MAP + 36,
    TILE_TRACK_TURF_CROWD_1          = TILE_COLOR_MAP + 37,
    TILE_TRACK_TURF_FINISH           = TILE_COLOR_MAP + 38,
    TILE_TRACK_TURF_FURLONG          = TILE_COLOR_MAP + 39,
    TILE_TRACK_TURF_GRANDSTAND_0     = TILE_COLOR_MAP + 40,
    TILE_TRACK_TURF_GRANDSTAND_1     = TILE_COLOR_MAP + 41,
    TILE_TRACK_TURF_GRANDSTAND_RUN   = TILE_COLOR_MAP + 42,
    TILE_TRACK_TURF_FINISH_ALT       = TILE_COLOR_MAP + 43,
    TILE_TRACK_TURF_FURLONG_ALT      = TILE_COLOR_MAP + 44,
    TILE_TRACK_TURF_HORSE_LIGHT      = TILE_COLOR_MAP + 45,
    TILE_TRACK_TURF_HORSE_GOLD       = TILE_COLOR_MAP + 46,
    TILE_TRACK_TURF_MARKER           = TILE_COLOR_MAP + 47,

    TILE_TRACK_UTIL_DRY_0            = TILE_COLOR_MAP + 48,
    TILE_TRACK_UTIL_DRY_1            = TILE_COLOR_MAP + 49,
    TILE_TRACK_UTIL_DRY_2            = TILE_COLOR_MAP + 50,
    TILE_TRACK_UTIL_DRY_3            = TILE_COLOR_MAP + 51,
    TILE_TRACK_UTIL_TURF_CROWD_0     = TILE_COLOR_MAP + 52,
    TILE_TRACK_UTIL_TURF_CROWD_1     = TILE_COLOR_MAP + 53,
    TILE_TRACK_UTIL_TURF_CROWD_2     = TILE_COLOR_MAP + 54,
    TILE_TRACK_UTIL_TURF_CROWD_3     = TILE_COLOR_MAP + 55,
    TILE_TRACK_UTIL_POST_0           = TILE_COLOR_MAP + 56,
    TILE_TRACK_UTIL_POST_1           = TILE_COLOR_MAP + 57,
    TILE_TRACK_UTIL_POST_2           = TILE_COLOR_MAP + 58,
    TILE_TRACK_UTIL_POST_3           = TILE_COLOR_MAP + 59,
    TILE_TRACK_UTIL_RAIL_CURVE_0     = TILE_COLOR_MAP + 60,
    TILE_TRACK_UTIL_RAIL_CURVE_1     = TILE_COLOR_MAP + 61,
    TILE_TRACK_UTIL_RAIL_CURVE_2     = TILE_COLOR_MAP + 62,
    TILE_TRACK_UTIL_RAIL_CURVE_3     = TILE_COLOR_MAP + 63,
};

#define PAL_TITLE    32  // title tileset palette:   16 colors (indices  32-47)
#define PAL_MAP      48  // map tileset palette:     64 colors (indices  48-111)
#define PAL_HUD     112  // hud tileset palette:      8 colors (indices 112-119)
#define PAL_SPRITES 120  // sprite tileset palette:  32 colors (indices 120-151)

enum FONT_FACE
{
    FONT_NICETY,
    FONT_NICETY_BOLD,
};

extern uint8_t tilemap_width;
extern uint8_t tilemap_height;

/// @brief Set the game state for the game main loop.
/// @param  state the game state to set.
void set_state(enum GAME_STATE state);

/// @brief immediately sets a palette color.
/// @param index palette entry to set, 0-255.
/// @param r red component of the color, 0-255.
/// @param g green component of the color, 0-255.
/// @param b blue component of the color, 0-255.
/// @note The color is converted to 565 format (5 bits red, 6 bits green, 5 bits blue).
void set_palette_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
/// @brief immediately sets a palette color.
/// @param index palette entry to set, 0-255.
/// @param color 565 color value to set the palette entry to.
void set_palette(uint8_t index, uint16_t color);
/// @brief File a tile in the tileset with a specific color.
/// @param tile the tile id from 0-255 to fill with the color.
/// @param color the palette index to fill the tile with, 0-255.
void fill_tileset(uint8_t tile, uint8_t color);
/// @brief draws a tile on on the off screen layer at the specified position.
/// @param x offset from the top left corner in tiles, 0-79 for the 16x16 tile.
/// @param y offset from the top left corner in tiles, 0-15 for the 16x16 tile.
/// @param tile the tile id from the tileset to draw, which must be in the tileset.
void draw_tilemap(uint16_t x, uint8_t y, uint8_t tile);
/// @brief Draws an array of tiles on the off screen tilemap at the specified position.
/// @param x top left position of the layer 
/// @param y top left position of the layer
/// @param tile pointer to an array of tile ids from the tileset to draw, which must be in the tileset.
/// @param count number of tiles to draw in the array, which must be less than or equal to 80.
void draw_tilemap_array(uint16_t x, uint8_t y, uint8_t *tile, uint8_t count);
/// @brief Fills a rectangular area of the off screen tilemap with a specific tile.
/// @param tile tile id from the tileset to fill the area with.
/// @param x position in tiles 0-79 for the 16x16 tile.
/// @param y position in tiles 0-15 for the 16x16 tile.
/// @param w tile count across (1-80)
/// @param h tile count down (1-16)
void fill_tilemap(uint8_t tile,uint16_t x, uint8_t y, uint8_t w, uint8_t h);
/// @brief  Renders the tilemap for a specific layer.
/// @param layer layer to render, 0 for the first layer, 1 for the second layer, etc.
/// @note The tilemap is rendered in a single pass, so it should be called after
void render_tilemap(uint8_t layer);
/// @brief Renders a single tile of the tilemap at the specified position for layer 0.
/// @param x 
/// @param y 
void render_tilemap_xy0(uint8_t x, uint8_t y);
/// @brief Renders a single tile of the tilemap at the specified position for layer 1.
/// @param x
/// @param y
void render_tilemap_xy1(uint8_t x, uint8_t y);
/// @brief Renders a single tile of the tilemap at the specified position for layer 1, using a label string instead of the tilemap buffer.
/// @param x column to render the tile at, 0-79 for the 16x16 tile.
/// @param y row to render the tile at, 0-15 for the 16x16 tile.
/// @param label pointer to an array of tile ids from the tileset to draw, which must be in the tileset.
/// @param len number of tiles to draw in the array, which must be less than or
void render_tilemap_label1(uint8_t x, uint8_t y, const char *label, uint8_t len);
/// @brief Clear the off screen sprite buffer to be rendered later.  Fully zeros the buffer, and sets the current count to 0.
void reset_sprite(void);
/// @brief Adds a sprite to the sprite buffer to be rendered later.
/// @param x pixel position - 16 of the sprite, anchored to the top left corner.
/// @param y pixel position - 16 of the sprite, anchored to the top left corner.
/// @param sprite tile number of the sprite to render, which must be in the tileset.
/// @note all sprites are 16x16 pixels and the position is the top left corner of the sprite.
void add_sprite(uint16_t x, uint8_t y, uint8_t sprite);
/// @brief Renders all sprites in the sprite buffer to the screen.
void render_sprites(void);
/// @brief Clears all sprites by resetting the sprites, the rendering the empty sprites
void clear_sprites(void);

#define MAX_TEXT_TILES 16
/// @brief Set the offscreen text tiles to a specific color.
/// @note Each text tile is 16x16 pixels.  And there are 16 of them.  The fonts are 8x8, so 2x2 tiles.
/// @param color palette index for the text tiles.
void clear_text_tiles(uint8_t color, uint8_t count);
/// @brief Set the font to be used for text rendering
void set_font(enum FONT_FACE font);
/// @brief Draw a single pixel in the offscreen text tilemap.
/// @param x horizontal values 0-255 with clipping, with 0 being the left.
/// @param y vertical values 0-15, with 0 being the top.
/// @param color palette index for the pixel.
void draw_text_pixel(uint16_t x, uint8_t y, uint8_t color);
/// @brief Draw a character in the offscreen text tilemap.
/// @param x horizontal values 0-255 with clipping, with 0 being the left.
/// @param y vertical values 0-15, with 0 being the top.
/// @param c character to draw, which must be in the font.
/// @param color palette index for the character.
void draw_text(uint16_t x, uint8_t y, const char *text, uint8_t color);
/// @brief Draw a character in the offscreen text tilemap with a background color.
/// @param x horizontal values 0-255 with clipping, with 0 being the left.
/// @param y vertical values 0-15, with 0 being the top.
/// @param c character to draw, which must be in the font.
/// @param fg palette index for the character.
/// @param bg palette index for the background.
void draw_text_opaque(uint16_t x, uint8_t y, const char *text, uint8_t fg, uint8_t bg);
/// @brief Renders the text tiles to the screen from the offscreen buffer.
/// @param tile tile id to start rendering from, which must be in the tileset.
/// @param tile_count number of tiles to render, which must be less than or equal to 16.
void render_text(uint8_t tile, uint8_t tile_count);
/// @brief Renders a subset of the text tiles to the screen.
/// @param base_tile absolute tileset slot of the first tile in the strip (e.g. DIALOG_TILE).
/// @param tile_offset 0-based index within the strip of the first tile to upload.
/// @param count number of consecutive tiles to upload.
void render_text_offset(uint8_t base_tile, uint8_t tile_offset, uint8_t count);

/// @brief Log to the serial port.
/// @param message the message to log.
void debug_log(const char *message);
/// @brief Log to the serial port with a format string.
/// @param format the format string to use.
/// @param ... the arguments to the format string.
void debug_logf(const char *format, ...);

#endif
