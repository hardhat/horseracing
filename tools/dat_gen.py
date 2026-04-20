#!/usr/bin/env python3
"""
dat_gen.py — Build the current template asset pack, bin/horser.dat.

The filename and asset grouping still come from the inherited sample project.
Replace them when Horse Racing art is ready.

Layout
------
    [DAT_BLOCK_COUNT × DatEntry]   header (72 bytes, entry 0 is magic cookie)
  [title tileset]
  [title palette]
    [map tileset]
    [map palette]
  [hud tileset]
  [hud palette]
    [sprites tileset]
    [sprites palette]

Run from the repository root:
    python3 tools/dat_gen.py
"""

import os
import struct

# ---------------------------------------------------------------------------
# Constants — must match dat.h
# ---------------------------------------------------------------------------

DAT_MAGIC = 0x534F5248          # 'H','O','R','S' as little-endian uint32 — must match dat.h

DAT_TYPE_MAGIC_COOKIE = 0
DAT_TYPE_TILESET      = 1
DAT_TYPE_PALETTE      = 2


DAT_BLOCK_MAGIC         = 0
DAT_BLOCK_TITLE_TS    = 1
DAT_BLOCK_TITLE_PAL   = 2
DAT_BLOCK_MAP_TS      = 3
DAT_BLOCK_MAP_PAL     = 4
DAT_BLOCK_HUD_TS     = 5
DAT_BLOCK_HUD_PAL    = 6
DAT_BLOCK_SPRITES_TS     = 7
DAT_BLOCK_SPRITES_PAL    = 8
DAT_BLOCK_THEME_TS     = 9
DAT_BLOCK_THEME_PAL    = 10
DAT_BLOCK_ACCENT_TS    = 11
DAT_BLOCK_ACCENT_PAL   = 12
DAT_BLOCK_COUNT         = 13

# DatEntry: uint32 offset, uint16 size, uint8 type, uint8 id
ENTRY_FMT  = '<IHBB'
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)   # 8 bytes
HEADER_SIZE = DAT_BLOCK_COUNT * ENTRY_SIZE  # 72 bytes

# ---------------------------------------------------------------------------
# DatEntry helpers
# ---------------------------------------------------------------------------

def make_entry(offset, size, entry_type, entry_id):
    return struct.pack(ENTRY_FMT, offset, size, entry_type, entry_id)


def make_magic_entry():
    return make_entry(DAT_MAGIC, HEADER_SIZE, DAT_TYPE_MAGIC_COOKIE, 0)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    img_dir    = os.path.join(base, 'img')
    out_path   = os.path.join(base, 'bin', 'horser.dat')

    # Template asset pairs: (block_id_ts, block_id_pal, stem)
    # TODO: Replace these stems with horse-racing groups when new art exists.
    asset_pairs = [
        (DAT_BLOCK_TITLE_TS,   DAT_BLOCK_TITLE_PAL,   'title'),
        (DAT_BLOCK_MAP_TS,     DAT_BLOCK_MAP_PAL,     'map'),
        (DAT_BLOCK_HUD_TS,     DAT_BLOCK_HUD_PAL,     'hud'),
        (DAT_BLOCK_SPRITES_TS, DAT_BLOCK_SPRITES_PAL, 'sprites'),
        (DAT_BLOCK_THEME_TS,   DAT_BLOCK_THEME_PAL,   'theme'),
        (DAT_BLOCK_ACCENT_TS,  DAT_BLOCK_ACCENT_PAL,  'accent'),
    ]

    # ------------------------------------------------------------------
    # Load art assets
    # ------------------------------------------------------------------
    asset_data = {}   # block_id -> bytes
    for (bid_ts, bid_pal, stem) in asset_pairs:
        ts_path  = os.path.join(img_dir, f'{stem}.zts')
        pal_path = os.path.join(img_dir, f'{stem}.ztp')
        with open(ts_path, 'rb') as f:
            asset_data[bid_ts] = f.read()
        with open(pal_path, 'rb') as f:
            asset_data[bid_pal] = f.read()
        print(f'  {stem}: tileset {len(asset_data[bid_ts])} bytes, '
              f'palette {len(asset_data[bid_pal])} bytes')

    # ------------------------------------------------------------------
    # Build header
    # ------------------------------------------------------------------
    # Calculate data offsets (data starts after header)
    data_offset = HEADER_SIZE
    block_offsets = {}
    for bid in range(1, DAT_BLOCK_COUNT):   # skip magic cookie slot
        block_offsets[bid] = data_offset
        data_offset += len(asset_data[bid])

    header = bytearray()
    header += make_magic_entry()
    for bid in range(1, DAT_BLOCK_COUNT):
        data = asset_data[bid]
        if bid in [DAT_BLOCK_TITLE_TS, DAT_BLOCK_MAP_TS, DAT_BLOCK_HUD_TS, DAT_BLOCK_SPRITES_TS, DAT_BLOCK_THEME_TS, DAT_BLOCK_ACCENT_TS]:
            entry_type = DAT_TYPE_TILESET
        elif bid in [DAT_BLOCK_TITLE_PAL, DAT_BLOCK_MAP_PAL, DAT_BLOCK_HUD_PAL, DAT_BLOCK_SPRITES_PAL, DAT_BLOCK_THEME_PAL, DAT_BLOCK_ACCENT_PAL]:
            entry_type = DAT_TYPE_PALETTE
        header += make_entry(block_offsets[bid], len(data), entry_type, bid)

    assert len(header) == HEADER_SIZE

    # ------------------------------------------------------------------
    # Write output
    # ------------------------------------------------------------------
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'wb') as f:
        f.write(header)
        for bid in range(1, DAT_BLOCK_COUNT):
            f.write(asset_data[bid])

    total = HEADER_SIZE + sum(len(asset_data[bid]) for bid in range(1, DAT_BLOCK_COUNT))
    print(f'\nWrote {out_path}: {total} bytes total')


if __name__ == '__main__':
    main()
