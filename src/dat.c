#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <zos_vfs.h>

#include "dat.h"
#include "main.h"   // debug_log / debug_logf

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

static zos_dev_t    dat_fd = -1;
static DatEntry     header[DAT_BLOCK_COUNT];

// Palette data buffer — avoids Z80 stack pressure for up to 128-byte palettes.
static uint8_t pal_buf[128];

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static bool dat_seek(uint32_t offset)
{
    int32_t off = (int32_t)offset;
    return seek(dat_fd, &off, SEEK_SET) == 0;
}

static bool dat_read(void *buf, uint16_t size)
{
    // ZOS VFS read() must not cross a 16KB virtual page boundary.
    // Split into two reads if the destination buffer straddles one.
    uint16_t addr     = (uint16_t)(uintptr_t)buf;
    uint16_t page_rem = (uint16_t)(0x4000u - (addr & 0x3FFFu));

    if (page_rem >= size) {
        uint16_t n = size;
        return read(dat_fd, buf, &n) == 0 && n == size;
    }

    // First chunk: up to the page boundary
    uint16_t n = page_rem;
    if (read(dat_fd, buf, &n) != 0 || n != page_rem) return false;

    // Second chunk: remainder at start of next page
    n = size - page_rem;
    return read(dat_fd, (uint8_t *)buf + page_rem, &n) == 0 && n == (size - page_rem);
}

// ---------------------------------------------------------------------------
// dat_open — open the current template asset pack and validate the header.
// TODO: Rename the file once the Horse Racing asset pipeline replaces the
// inherited sample assets.
// ---------------------------------------------------------------------------

bool dat_open(void)
{
    dat_fd = open("horser.dat", O_RDONLY);
    if (dat_fd < 0) {
        debug_log("dat: failed to open template asset pack (horser.dat)");
        return false;
    }

    // Read full header in one shot.
    uint16_t hdr_size = sizeof(header);
    if (read(dat_fd, header, &hdr_size) != 0 || hdr_size != sizeof(header)) {
        debug_log("dat: header read failed");
        close(dat_fd);
        dat_fd = -1;
        return false;
    }

    // Validate magic cookie entry.
    if (header[DAT_BLOCK_MAGIC].offset != DAT_MAGIC ||
        header[DAT_BLOCK_MAGIC].size   != sizeof(header) ||
        header[DAT_BLOCK_MAGIC].type   != DAT_TYPE_MAGIC_COOKIE) {
        debug_log("dat: bad magic cookie");
        close(dat_fd);
        dat_fd = -1;
        return false;
    }

    debug_log("dat: opened template asset pack (horser.dat)");
    return true;
}

// ---------------------------------------------------------------------------
// dat_load_tileset
// ---------------------------------------------------------------------------

bool dat_load_tileset(uint8_t block_id, uint8_t *dst, uint16_t *size_out)
{
    if (dat_fd < 0 || block_id >= DAT_BLOCK_COUNT) return false;

    DatEntry *e = &header[block_id];
    if (e->type != DAT_TYPE_TILESET) return false;

    if (!dat_seek(e->offset)) return false;

    if (!dat_read(dst, e->size)) return false;

    if (size_out) *size_out = e->size;
    return true;
}

// ---------------------------------------------------------------------------
// dat_load_palette
// ---------------------------------------------------------------------------

bool dat_load_palette(uint8_t block_id, uint8_t **dst_out, uint16_t *size_out)
{
    if (dat_fd < 0 || block_id >= DAT_BLOCK_COUNT) return false;

    DatEntry *e = &header[block_id];
    if (e->type != DAT_TYPE_PALETTE) return false;
    if (e->size > sizeof(pal_buf)) return false;

    if (!dat_seek(e->offset)) return false;

    if (!dat_read(pal_buf, e->size)) return false;

    if (dst_out)  *dst_out  = pal_buf;
    if (size_out) *size_out = e->size;
    return true;
}
