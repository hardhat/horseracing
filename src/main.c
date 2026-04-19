#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdarg.h>

#ifndef __SDCC_VERSION_MAJOR
#define __at(addr)
#define __naked
#define __sfr
#define va_list struct {int dummy; }
#define va_start(ap, last)
#define va_end(ap)
#endif

#include<zos_sys.h>
#include<zos_keyboard.h>
#include<zos_vfs.h>
#include<zos_video.h>
#include<zvb_hardware.h>
#include<zvb_gfx.h>
#include<zvb_sprite.h>
#include<zvb_sound.h>

//#include "Flamboyant.h"
#include "Nicety.h"
#include "Nicety Bold.h"

#include "main.h"
#include "menu.h"
#include "game.h"
#include "dat.h"
#include "controller.h"
#include "textaccel.h"

void draw_big_text(const char *text, uint16_t x, uint8_t y, uint8_t color);

enum GAME_STATE game_state = GAME_STATE_MENU;

gfx_context ctx;

bool pressed = true;
bool quit = false;

uint8_t sprite_count=0;
uint8_t last_sprite_count=0;
gfx_sprite sprites[128];

// Load buffer — pinned at 0xD000 (page boundary) so reads never cross a page.
// Sized to hold the largest tileset (map.zts 2KB compressed).
// Also used for text tile rendering (max 20 tiles × 256 bytes = 5 KB).
__at(0xE000) uint8_t text_tiles[5120];
const uint8_t *font = FONT_NICETY_BITMAP;

uint8_t tilemap0[20*15];
//uint8_t tilemap1[20*15];
uint8_t tilemap_width=20;
uint8_t tilemap_height=15;

zos_dev_t ser;
uint16_t controller_state;

void debug_log(const char *message)
{
    size_t size=strlen(message);

    write(ser, message, &size);
    size=2;
    write(ser, "\r\n", &size);
}

void debug_logf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    debug_log(buffer);
}

void set_state(enum GAME_STATE state)
{
    switch(state)
    {
        case GAME_STATE_MENU:
            init_menu();
            break;
        case GAME_STATE_GAME:
            init_game();
            break;
        case GAME_STATE_GAMEOVER:
            init_gameover();
            break;
    }
    debug_logf("Setting state to %d.", state);
    game_state = state;
}

uint8_t handle_input(uint8_t key)
{
    switch(key)
    {
        case KB_ESC:
            quit = true;
            return MAX_INPUT;
        case KB_KEY_W:
        case KB_UP_ARROW:
            return INPUT_UP;
        case KB_KEY_S:
        case KB_DOWN_ARROW:
            return INPUT_DOWN;
        case KB_KEY_A:
        case KB_LEFT_ARROW:
            return INPUT_LEFT;
        case KB_KEY_D:
        case KB_RIGHT_ARROW:
            return INPUT_RIGHT;
        case KB_KEY_V:
        case KB_KEY_SPACE:
            return INPUT_A;
        case KB_KEY_BACKSPACE:
        case KB_KEY_B:
            return INPUT_B;
        case KB_KEY_COMMA:
        case KB_KEY_X:
            return INPUT_X;
        case KB_KEY_PERIOD:
        case KB_KEY_Y:
            return INPUT_Y;
        case KB_KEY_ENTER:
            return INPUT_START;
        case KB_KEY_QUOTE:
        case KB_RIGHT_SHIFT:
            return INPUT_SELECT;
        case KB_KEY_LEFT_BRACKET:
        case KB_KEY_Q:
            return INPUT_L;
        case KB_KEY_RIGHT_BRACKET:
        case KB_KEY_E:
            return INPUT_R;
        default:
            return MAX_INPUT;
    }
}

void init(void)
{
    ser = open("#SER0",O_WRONLY);
    if (ser < 0) {
        printf("Failed to open serial port\n");
	exit(1);
    }
    debug_log("Initializing...");
    controller_init();

    gfx_enable_screen(0);
    gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &ctx);

    memset(tilemap0, 0, sizeof(tilemap0));
    render_tilemap(1);

    gfx_tileset_add_color_tile(&ctx, TILE_COLOR_BLACK, TEXT_COLOR_BLACK);
    gfx_tileset_add_color_tile(&ctx, TILE_COLOR_BLUE, TEXT_COLOR_BLUE);

    set_font(FONT_NICETY);
    draw_big_text("HR", 2, 4, TILE_COLOR_BLUE);
    render_tilemap(0);
    gfx_enable_screen(1);

    zvb_sound_initialize(1);

    if (!dat_open()) {
        debug_log("Failed to open template asset pack (horser.dat)");
        printf("Failed to find template asset pack horser.dat\nMake sure it is in the same directory as the executable.\n");
        zvb_sound_reset();
        ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
        close(ser);

        exit(1);
    }

    uint16_t ts_size;
    uint8_t *pal_ptr;
    uint16_t pal_size;

    // Template asset groups retained from the sample project.
    // TODO: Rename slots and replace art once horse-racing assets exist.
    gfx_tileset_options options0 = {TILESET_COMP_LZ, (uint16_t)TILE_COLOR_TITLE*256u, PAL_TITLE, 0};
    dat_load_tileset(DAT_BLOCK_TITLE_TS, text_tiles, &ts_size);
    dat_load_palette(DAT_BLOCK_TITLE_PAL, &pal_ptr, &pal_size);
    gfx_palette_load(&ctx, pal_ptr, pal_size, PAL_TITLE);
    gfx_tileset_load(&ctx, text_tiles, ts_size, &options0);
    for(int i=0;i<17;i++) {
        draw_tilemap(2+i,0,TILE_COLOR_TITLE+i);
        draw_tilemap(2+i,1,TILE_COLOR_TITLE+i+17);
    }
    draw_big_text("HR", 2, 4, TILE_COLOR_TITLE+17);
    render_tilemap(0);

    gfx_tileset_options options1 = {TILESET_COMP_LZ, (uint16_t)TILE_COLOR_MAP*256u, PAL_MAP, 1};
    dat_load_tileset(DAT_BLOCK_MAP_TS, text_tiles, &ts_size);
    dat_load_palette(DAT_BLOCK_MAP_PAL, &pal_ptr, &pal_size);
    gfx_palette_load(&ctx, pal_ptr, pal_size, PAL_MAP);
    gfx_tileset_load(&ctx, text_tiles, ts_size, &options1);
    draw_big_text("HR", 2, 4, TILE_COLOR_MAP);
    for(int i=0;i<9;i++) {
        draw_tilemap(i+5,13,TILE_COLOR_MAP+i);
    }
    render_tilemap(0);

    gfx_tileset_options options2 = {TILESET_COMP_LZ, (uint16_t)TILE_COLOR_HUD*256u, PAL_HUD, 1};
    dat_load_tileset(DAT_BLOCK_HUD_TS, text_tiles, &ts_size);
    dat_load_palette(DAT_BLOCK_HUD_PAL, &pal_ptr, &pal_size);
    gfx_palette_load(&ctx, pal_ptr, pal_size, PAL_HUD);
    gfx_tileset_load(&ctx, text_tiles, ts_size, &options2);
    draw_big_text("HR", 2, 4, TILE_COLOR_HUD);
    for(int i=0;i<14;i++) {
        draw_tilemap(i+2,14,TILE_COLOR_HUD+i);
    }
    render_tilemap(0);

    gfx_tileset_options options3 = {TILESET_COMP_LZ, (uint16_t)TILE_COLOR_SPRITES*256u, PAL_SPRITES, 1};
    dat_load_tileset(DAT_BLOCK_SPRITES_TS, text_tiles, &ts_size);
    dat_load_palette(DAT_BLOCK_SPRITES_PAL, &pal_ptr, &pal_size);
    gfx_palette_load(&ctx, pal_ptr, pal_size, PAL_SPRITES);
    gfx_tileset_load(&ctx, text_tiles, ts_size, &options3);
    draw_big_text("HR", 2, 4, TILE_COLOR_SPRITES);
    render_tilemap(0);

    debug_log("Loaded tilesets and palettes.");

    // Build a single-tile betting cursor marker instead of the old 2x2 box.
    clear_text_tiles(TEXT_COLOR_BLACK, 1);
    for(int i=3;i<13;i++)
    {
        draw_text_pixel(13, (uint8_t)i, TEXT_COLOR_WHITE);
    }
    for(int i=0;i<5;i++)
    {
        draw_text_pixel((uint16_t)(13 - i), (uint8_t)(7 - i), TEXT_COLOR_WHITE);
        draw_text_pixel((uint16_t)(13 - i), (uint8_t)(8 + i), TEXT_COLOR_WHITE);
    }
    for(int i=0;i<4;i++)
    {
        draw_text_pixel((uint16_t)(5 + i), 7, TEXT_COLOR_WHITE);
        draw_text_pixel((uint16_t)(5 + i), 8, TEXT_COLOR_WHITE);
    }
    render_text(TILE_CURSOR, 1);

    gfx_enable_screen(0);

    memset(tilemap0, 0, sizeof(tilemap0));
    render_tilemap(0);
    render_tilemap(1);

    gfx_enable_screen(1);

    /* Initialize the keyboard by setting it to raw and non-blocking */
    void* arg = (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);

    debug_log("Initialized.");
}

void set_palette(uint8_t index, uint16_t color)
{
    gfx_palette_load(&ctx, &color, 1, index);
}

void set_palette_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t color = RGB888_TO_RGB565(r, g, b);
    gfx_palette_load(&ctx, &color, 1, index);
}

void fill_tileset(uint8_t tile, uint8_t color)
{
    gfx_tileset_add_color_tile(&ctx, tile, color);
}

void clear_text_tiles(uint8_t color, uint8_t count)
{
    if (count > 20) count = 20;
    memset(text_tiles, color, (size_t)count * 256);
}

void set_font(enum FONT_FACE face)
{
    switch(face)
    {
        case FONT_NICETY:
            font = FONT_NICETY_BITMAP;
            break;
        case FONT_NICETY_BOLD:
            font = FONT_NICETY_BOLD_BITMAP;
            break;
    }
}

void draw_big_text_pixel(uint16_t x, uint8_t y, uint8_t color)
{
    int offset = x+y*tilemap_width;
    if(offset<0 || offset>=tilemap_width*tilemap_height) return;  // Clip
    tilemap0[offset] = color;
}   

void draw_big_text_char(uint16_t x, uint8_t y, uint8_t c, uint8_t color)
{
    for(uint8_t i=0;i<8;i++)
    {
        uint8_t mask = 1<<(7-i);
        for(uint8_t j=0;j<8;j++)
        {
            if(font[(c-32)*8+j] & mask)
                draw_big_text_pixel(x+i, y+j, color);
        }
    }
}

void draw_big_text(const char *text, uint16_t x, uint8_t y, uint8_t color)
{
    for(int i=0;i<strlen(text);i++)
    {
        uint8_t c = text[i];
        if(c<32 || c>127) c=32;
        draw_big_text_char(x+i*8, y, c, color);
    }
}

void draw_text_pixel(uint16_t x, uint8_t y,uint8_t color)
{
    //int offset = (x/16)*256+y*16+(x%16);
    uint16_t offset = ((x>>4)<<8)+(y<<4)+(x&0x0F);
    if(offset>=256*20) return;  // Clip
    text_tiles[offset] = color;
}

void draw_text_pixel_offset(uint16_t offset, uint8_t color)
{
    if(offset>=256*20) return;  // Clip
    text_tiles[offset] = color;
}

void draw_text_char(uint16_t x, uint8_t y, uint8_t c, uint8_t color)
{
    if(((x&15)>8) || ((y&15)>8)) {
        // Straddling tiles, draw pixel by pixel
        const uint8_t *ptr = font+((c-32)<<3);
        for(uint8_t i=0;i<8;i++)
        {
            uint8_t mask = 1<<(7-i);
            for(uint8_t j=0;j<8;j++)
            {
                if(ptr[j] & mask)
                    draw_text_pixel(x+i, y+j, color);
            }
        }
    } else {
        // Save some computation on the common case of aligned text by precomputing the offset and only checking tile bounds once.
        uint16_t offset = ((x>>4)<<8)+(y<<4)+(x&0x0F);
        const uint8_t *ptr = font+((c-32)<<3);
        draw_text_char_aligned(ptr, &text_tiles[offset], color);
    }
}

void draw_text(uint16_t x, uint8_t y, const char *text, uint8_t color)
{
    for(int i=0;i<strlen(text);i++)
    {
        uint8_t c = text[i];
        if(c<32 || c>127) c=32;
        draw_text_char(x+i*8, y, c, color);
    }
}

void draw_char_opaque(uint16_t x, uint8_t y, uint8_t c, uint8_t fg, uint8_t bg)
{
    if(c<32 || c>127) c=32;
    const uint8_t *ptr = font+((c-32)<<3);
    if(((x&15)<=8) && ((y&15)<=8)) {
        // use faster offset instead.
        uint16_t offset=((x>>4)<<8)+(y<<4)+(x&0x0F);
#if 0
        for(uint8_t j=0;j<8;j++) {
            uint8_t mask=0x80;
            uint8_t pat=ptr[j]; // get the pixel pattern
            for(uint8_t i=0;i<8;i++) {
                if(pat & mask)
                    draw_text_pixel_offset(offset, fg);
                else
                    draw_text_pixel_offset(offset, bg);
                offset++;
                mask>>=1;
            }
            offset+=8; // 8 to move to the next line
        }
#else
        draw_text_char_opaque_aligned(ptr, &text_tiles[offset], fg, bg);
#endif
        return;
    }
            
    for(uint8_t i=0;i<8;i++)
    {
        uint8_t mask = 1<<(7-i);
        for(uint8_t j=0;j<8;j++)
        {
            if(ptr[j] & mask)
                draw_text_pixel(x+i, y+j, fg);
            else
                draw_text_pixel(x+i, y+j, bg);
        }
    }
}

void draw_text_opaque(uint16_t x, uint8_t y, const char *text, uint8_t fg, uint8_t bg)
{
    for(int i=0;i<strlen(text);i++)
    {
        uint8_t c = text[i];
        draw_char_opaque(x, y, c, fg, bg);
        x+=8;
    }
}

void render_text(uint8_t tile, uint8_t tile_count)
{
    uint16_t from = tile<<8;
    gfx_tileset_options options = {0,from,0,0};
    gfx_tileset_load(&ctx, text_tiles, tile_count<<8, &options);
}

void render_text_offset(uint8_t base_tile, uint8_t tile_offset, uint8_t count)
{
    uint16_t from = ((uint16_t)base_tile + tile_offset) << 8;
    gfx_tileset_options options = {0, from, 0, 0};
    gfx_tileset_load(&ctx, text_tiles + ((uint16_t)tile_offset << 8),
                     (uint16_t)count << 8, &options);
}

void draw_tilemap(uint16_t x, uint8_t y, uint8_t tile)
{
    tilemap0[y*tilemap_width+x] = tile;
}

void draw_tilemap_array(uint16_t x, uint8_t y, uint8_t *tile, uint8_t count)
{
    for(int i=0;i<count;i++)
    {
        tilemap0[y*tilemap_width+x+i] = tile[i];
    }
}

void fill_tilemap(uint8_t tile,uint16_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint16_t offset = (y*tilemap_width)+x;
    if(y+h>tilemap_height) h=tilemap_height-y;
    if(x+w>tilemap_width) w=tilemap_width-x;
    for(int i=0;i<h;i++)
    {
        for(int j=0;j<w;j++)
        {
            tilemap0[offset+j] = tile;
        }
        offset += tilemap_width;
    }
}

void render_tilemap(uint8_t layer)
{
    for(int y=0;y<tilemap_height;y++)
    {
        gfx_tilemap_load(&ctx, tilemap0+y*tilemap_width, tilemap_width, layer, 0, y);
    }
}

void render_tilemap_xy0(uint8_t x, uint8_t y)
{
    gfx_tilemap_load(&ctx, tilemap0+y*tilemap_width+x, 1, 0, x, y);
}

void render_tilemap_xy1(uint8_t x, uint8_t y)
{
    gfx_tilemap_load(&ctx, tilemap0+y*tilemap_width+x, 1, 1, x, y);
}

void render_tilemap_label1(uint8_t x, uint8_t y, const char *label, uint8_t len)
{
    gfx_tilemap_load(&ctx, label, len, 1, x, y);
}

void set_layer_scroll(uint8_t layer, uint16_t x, uint16_t y)
{
    switch(layer)
    {
        case 0u:
            zvb_ctrl_l0_scr_x_low = (uint8_t)(x & 0x00FFu);
            zvb_ctrl_l0_scr_x_high = (uint8_t)(x >> 8);
            zvb_ctrl_l0_scr_y_low = (uint8_t)(y & 0x00FFu);
            zvb_ctrl_l0_scr_y_high = (uint8_t)(y >> 8);
            break;

        case 1u:
            zvb_ctrl_l1_scr_x_low = (uint8_t)(x & 0x00FFu);
            zvb_ctrl_l1_scr_x_high = (uint8_t)(x >> 8);
            zvb_ctrl_l1_scr_y_low = (uint8_t)(y & 0x00FFu);
            zvb_ctrl_l1_scr_y_high = (uint8_t)(y >> 8);
            break;

        default:
            break;
    }
}

void reset_layer_scroll(uint8_t layer)
{
    set_layer_scroll(layer, 0u, 0u);
}

void reset_layer_scrolls(void)
{
    reset_layer_scroll(0u);
    reset_layer_scroll(1u);
}

void reset_sprite(void)
{
    last_sprite_count = sprite_count;
    sprite_count=0;
    memset(sprites, 0, sizeof(sprites));
}

void add_sprite(uint16_t x, uint8_t y, uint8_t sprite)
{
    if(sprite_count >= 128) return;
    sprites[sprite_count].x = x+16; // Note sprites are displayed anchored the bottom right corner
    sprites[sprite_count].y = y+16;
    sprites[sprite_count].tile = sprite;
    sprites[sprite_count].flags = 0;
    sprite_count++;
}

void render_sprites(void)
{
    uint8_t count = sprite_count;
    if(count < last_sprite_count) count = last_sprite_count;
    gfx_sprite_render_array(&ctx, 0, sprites, count);
}

void clear_sprites(void)
{
    reset_sprite();
    sprite_count=128;
    render_sprites();
    sprite_count=0;
}

// Map from input bit to INPUT_ enum value
const enum INPUT map_controller[]={
    INPUT_B, // 0
    INPUT_Y, // 1
    INPUT_SELECT, // 2
    INPUT_START, // 3
    INPUT_UP, // 4
    INPUT_DOWN, // 5
    INPUT_LEFT, // 6
    INPUT_RIGHT, // 7
    INPUT_A, // 8
    INPUT_X, // 9
    INPUT_L, // 10
    INPUT_R, // 11
};

void send_input(uint8_t input, bool down)
{
    switch(game_state)
    {
        case GAME_STATE_MENU:
            input_menu(input, down);
            break;
        case GAME_STATE_GAME:
            input_game(input, down);
            break;
        case GAME_STATE_GAMEOVER:
            input_gameover(input, down);
            break;
    }
}

void process_input(void)
{
    unsigned char keys[32];
    int size;

    do {
        size=32;
        read(DEV_STDIN, &keys, &size);
        for(int i=0;i<size;i++) {
            char key = keys[i];
           //debug_logf("Processing input key %02x.", key);
           if(key == KB_RELEASED) {
                pressed = false;
            } else {
                uint8_t input = handle_input(key);
                if(input >= MAX_INPUT) {
                    pressed = true;
                    continue;
                }
                send_input(input, pressed);
                pressed=true;
            }
        }
    } while(size>0);
    // Scan for game controller
    uint16_t value = controller_read();
    uint16_t changed = value ^ controller_state;
    if(changed) debug_logf("Ctrl: %04x, %04x", value, changed);
    controller_state = value;
    // Edge triggered
    for(uint8_t i=0;i<12;i++) {
        if(changed & (1<<i)) {
            uint8_t input = map_controller[i];
            bool down = (value & (1<<i)) == 0;
            send_input(input, down);
        }
    }
}

int main(void)
{
    init();

    set_state(GAME_STATE_MENU);

    while(!quit)
    {
        process_input();

        gfx_wait_end_vblank(&ctx);
        switch(game_state)
        {
            case GAME_STATE_MENU:
                update_menu();
                break;
            case GAME_STATE_GAME:
                update_game();
                break;
            case GAME_STATE_GAMEOVER:
                update_gameover();
                break;
        }
        gfx_wait_vblank(&ctx);
        switch(game_state) {
            case GAME_STATE_MENU:
                draw_menu();
                break;
            case GAME_STATE_GAME:
                draw_game();
                break;
            case GAME_STATE_GAMEOVER:
                draw_gameover();
                break;
        }
    }
    debug_log("Quitting.");

    // Clear out sprites
    memset(sprites, 0, sizeof(sprites));
    render_sprites();
    zvb_sound_reset();
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    printf("Exiting...\n");

    printf("Goodbye!\n");
    close(ser);

    exit(0);

    return 0;
}
