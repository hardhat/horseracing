#ifndef DAT_H
#define DAT_H

#include <stdbool.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Template asset pack. The filename is horser.dat
// ---------------------------------------------------------------------------

#define DAT_MAGIC           0x534F5248UL  /* 'H','O','R','S' little-endian */

#define DAT_TYPE_MAGIC_COOKIE  0
#define DAT_TYPE_TILESET       1
#define DAT_TYPE_PALETTE       2
#define DAT_TYPE_SCRIPTS       3
#define DAT_TYPE_STRINGS       4

#define DAT_BLOCK_MAGIC         0
#define DAT_BLOCK_TITLE_TS    1
#define DAT_BLOCK_TITLE_PAL   2
#define DAT_BLOCK_MAP_TS    3
#define DAT_BLOCK_MAP_PAL   4
#define DAT_BLOCK_HUD_TS     5
#define DAT_BLOCK_HUD_PAL    6
#define DAT_BLOCK_SPRITES_TS     7
#define DAT_BLOCK_SPRITES_PAL    8
#define DAT_BLOCK_COUNT         9

// On-disk header entry (8 bytes, packed).
// Entry 0 is always the magic cookie:
//   offset = DAT_MAGIC ('VONT'), size = header size in bytes, type = DAT_TYPE_MAGIC_COOKIE
typedef struct {
    uint32_t offset;   // file byte offset of block data (or magic word for entry 0)
    uint16_t size;     // block size in bytes (or header size for entry 0)
    uint8_t  type;     // DAT_TYPE_*
    uint8_t  id;       // block id (DAT_BLOCK_*)
} DatEntry;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Open the template asset pack, read and validate the header. Call once at startup.
// Returns false and prints an error if the file is missing or corrupt.
bool dat_open(void);

// Load a tileset block into dst (use the existing text_tiles buffer).
// *size_out receives the number of bytes read.
bool dat_load_tileset(uint8_t block_id, uint8_t *dst, uint16_t *size_out);

// Load a palette block. Reads into an internal static buffer; the pointer
// returned via *dst_out is valid until the next dat_load_palette() call.
bool dat_load_palette(uint8_t block_id, uint8_t **dst_out, uint16_t *size_out);

#endif // DAT_H
