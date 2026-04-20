#!/usr/bin/env python3
from pathlib import Path

from PIL import Image, ImageDraw

TILE_SIZE = 16

IDX_BG_DARK = 0
IDX_BG_MID = 1
IDX_WHITE = 4
IDX_GOLD = 5
IDX_WOOD_DARK = 7
IDX_ORANGE = 13


def draw_bolt(draw: ImageDraw.ImageDraw, x: int, y: int) -> None:
    draw.rectangle((x - 1, y - 1, x + 1, y + 1), fill=IDX_GOLD)
    draw.point((x, y), fill=IDX_WHITE)


def draw_accent(draw: ImageDraw.ImageDraw) -> None:
    draw.rectangle((0, 0, 15, 15), fill=IDX_BG_DARK)
    draw.rectangle((1, 1, 14, 14), fill=IDX_WOOD_DARK)
    draw.rectangle((2, 2, 13, 13), outline=IDX_ORANGE)
    draw.rectangle((3, 3, 12, 12), fill=IDX_BG_MID)
    draw.rectangle((4, 4, 11, 11), fill=IDX_BG_DARK)
    draw.line((7, 4, 7, 11), fill=IDX_GOLD)
    draw.line((4, 7, 11, 7), fill=IDX_GOLD)
    draw_bolt(draw, 3, 3)
    draw_bolt(draw, 12, 3)
    draw_bolt(draw, 3, 12)
    draw_bolt(draw, 12, 12)


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    hud = Image.open(repo / 'img' / 'hud.gif')
    accent = Image.new('P', (TILE_SIZE, TILE_SIZE), IDX_BG_DARK)
    accent.putpalette(hud.getpalette())
    draw = ImageDraw.Draw(accent)

    draw_accent(draw)
    accent.save(repo / 'img' / 'accent.gif', save_all=False)


if __name__ == '__main__':
    main()