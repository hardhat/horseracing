#!/usr/bin/env python3
from pathlib import Path

from PIL import Image, ImageDraw

TILE_SIZE = 16
WIDTH_TILES = 4
HEIGHT_TILES = 3
WIDTH = TILE_SIZE * WIDTH_TILES
HEIGHT = TILE_SIZE * HEIGHT_TILES

IDX_BG_DARK = 0
IDX_BG_MID = 1
IDX_BG_LIGHT = 3
IDX_WHITE = 4
IDX_GOLD = 5
IDX_WOOD_LIGHT = 6
IDX_WOOD_DARK = 7
IDX_GREEN_DARK = 8
IDX_GREEN_MID = 9
IDX_GREEN_LIGHT = 10
IDX_TEAL = 11
IDX_BRICK = 12
IDX_ORANGE = 13


def tile_rect(tx: int, ty: int) -> tuple[int, int, int, int]:
    x0 = tx * TILE_SIZE
    y0 = ty * TILE_SIZE
    return (x0, y0, x0 + TILE_SIZE - 1, y0 + TILE_SIZE - 1)


def fill(draw: ImageDraw.ImageDraw, tx: int, ty: int, color: int) -> None:
    draw.rectangle(tile_rect(tx, ty), fill=color)


def draw_bolt(draw: ImageDraw.ImageDraw, x: int, y: int) -> None:
    draw.rectangle((x - 1, y - 1, x + 1, y + 1), fill=IDX_GOLD)
    draw.point((x, y), fill=IDX_WHITE)


def draw_wood_frame(draw: ImageDraw.ImageDraw, tx: int, ty: int, left: bool, top: bool, right: bool, bottom: bool) -> None:
    x0, y0, x1, y1 = tile_rect(tx, ty)
    draw.rectangle((x0, y0, x1, y1), fill=IDX_WOOD_DARK)
    if top:
        draw.rectangle((x0, y0, x1, y0 + 2), fill=IDX_ORANGE)
        draw.rectangle((x0, y0 + 3, x1, y0 + 4), fill=IDX_WOOD_LIGHT)
    if bottom:
        draw.rectangle((x0, y1 - 2, x1, y1), fill=IDX_ORANGE)
        draw.rectangle((x0, y1 - 4, x1, y1 - 3), fill=IDX_WOOD_LIGHT)
    if left:
        draw.rectangle((x0, y0, x0 + 2, y1), fill=IDX_ORANGE)
        draw.rectangle((x0 + 3, y0, x0 + 4, y1), fill=IDX_WOOD_LIGHT)
    if right:
        draw.rectangle((x1 - 2, y0, x1, y1), fill=IDX_ORANGE)
        draw.rectangle((x1 - 4, y0, x1 - 3, y1), fill=IDX_WOOD_LIGHT)


def draw_felt(draw: ImageDraw.ImageDraw, tx: int, ty: int, inset_left: int, inset_top: int, inset_right: int, inset_bottom: int) -> None:
    x0, y0, x1, y1 = tile_rect(tx, ty)
    draw.rectangle((x0 + inset_left, y0 + inset_top, x1 - inset_right, y1 - inset_bottom), fill=IDX_GREEN_DARK)
    for y in range(y0 + inset_top + 1, y1 - inset_bottom, 3):
        draw.line((x0 + inset_left + 1, y, x1 - inset_right - 1, y), fill=IDX_GREEN_MID)
    for y in range(y0 + inset_top + 2, y1 - inset_bottom, 6):
        for x in range(x0 + inset_left + 2, x1 - inset_right - 1, 6):
            draw.point((x, y), fill=IDX_GREEN_LIGHT)
    draw.rectangle((x0 + inset_left, y0 + inset_top, x1 - inset_right, y1 - inset_bottom), outline=IDX_BG_MID)


def draw_top_left(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(0, 0)
    draw_wood_frame(draw, 0, 0, True, True, False, False)
    draw_felt(draw, 0, 0, 4, 4, 0, 0)
    draw_bolt(draw, x0 + 11, y0 + 5)
    draw_bolt(draw, x0 + 5, y0 + 11)


def draw_top_fill(draw: ImageDraw.ImageDraw, tx: int, accent: bool) -> None:
    x0, y0, x1, y1 = tile_rect(tx, 0)
    draw_wood_frame(draw, tx, 0, False, True, False, False)
    draw_felt(draw, tx, 0, 0, 5, 0, 0)
    draw.line((x0 + 1, y0 + 10, x1 - 1, y0 + 10), fill=IDX_BG_DARK)
    if accent:
        for x in range(x0 + 2, x1 - 1, 4):
            draw.rectangle((x, y0 + 7, x + 1, y0 + 8), fill=IDX_GOLD)
        draw_bolt(draw, x0 + 3, y0 + 12)
        draw_bolt(draw, x1 - 3, y0 + 12)
    else:
        draw.rectangle((x0 + 2, y0 + 6, x1 - 2, y0 + 7), fill=IDX_BG_LIGHT)


def draw_top_right(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(3, 0)
    draw_wood_frame(draw, 3, 0, False, True, True, False)
    draw_felt(draw, 3, 0, 0, 4, 4, 0)
    draw_bolt(draw, x0 + 4, y0 + 5)
    draw_bolt(draw, x1 - 5, y0 + 11)


def draw_mid_left(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(0, 1)
    draw_wood_frame(draw, 0, 1, True, False, False, False)
    draw_felt(draw, 0, 1, 5, 0, 0, 0)
    draw.rectangle((x0 + 7, y0 + 2, x0 + 8, y1 - 2), fill=IDX_BG_LIGHT)
    draw_bolt(draw, x0 + 11, y0 + 4)
    draw_bolt(draw, x0 + 11, y1 - 4)


def draw_mid_fill(draw: ImageDraw.ImageDraw, tx: int, accent: bool) -> None:
    fill(draw, tx, 1, IDX_BG_DARK)
    x0, y0, x1, y1 = tile_rect(tx, 1)

    draw.rectangle((x0, y0, x1, y1), outline=IDX_BG_MID)
    draw.rectangle((x0 + 1, y0 + 1, x1 - 1, y1 - 1), fill=IDX_GREEN_DARK)
    draw.rectangle((x0 + 2, y0 + 2, x1 - 2, y1 - 2), fill=IDX_GREEN_MID)

    if accent:
        for y in range(y0 + 4, y1 - 2, 4):
            draw.line((x0 + 3, y, x1 - 3, y), fill=IDX_BG_MID)
        draw.line((x0 + 4, y0 + 7, x1 - 4, y0 + 7), fill=IDX_BG_LIGHT)
        draw.line((x0 + 4, y0 + 8, x1 - 4, y0 + 8), fill=IDX_BG_DARK)
    else:
        for y in range(y0 + 5, y1 - 1, 4):
            draw.line((x0 + 3, y, x1 - 3, y), fill=IDX_BG_MID)
        draw.line((x0 + 4, y0 + 6, x1 - 4, y0 + 6), fill=IDX_BG_LIGHT)
        draw.line((x0 + 4, y0 + 10, x1 - 4, y0 + 10), fill=IDX_BG_DARK)


def draw_mid_right(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(3, 1)
    draw_wood_frame(draw, 3, 1, False, False, True, False)
    draw_felt(draw, 3, 1, 0, 0, 5, 0)
    draw.rectangle((x0 + 7, y0 + 2, x0 + 8, y1 - 2), fill=IDX_BG_LIGHT)
    draw_bolt(draw, x0 + 4, y0 + 4)
    draw_bolt(draw, x0 + 4, y1 - 4)


def draw_bottom_left(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(0, 2)
    draw_wood_frame(draw, 0, 2, True, False, False, True)
    draw_felt(draw, 0, 2, 4, 0, 0, 4)
    draw_bolt(draw, x0 + 5, y0 + 4)
    draw_bolt(draw, x0 + 11, y1 - 5)


def draw_bottom_fill(draw: ImageDraw.ImageDraw, tx: int, accent: bool) -> None:
    x0, y0, x1, y1 = tile_rect(tx, 2)
    draw_wood_frame(draw, tx, 2, False, False, False, True)
    draw_felt(draw, tx, 2, 0, 0, 0, 5)
    if accent:
        for x in range(x0 + 2, x1 - 1, 4):
            draw.rectangle((x, y0 + 6, x + 1, y0 + 7), fill=IDX_GOLD)
        draw.rectangle((x0 + 3, y0 + 3, x1 - 3, y0 + 4), fill=IDX_BG_MID)
    else:
        draw.line((x0 + 2, y0 + 7, x1 - 2, y0 + 7), fill=IDX_BG_LIGHT)
        draw.line((x0 + 2, y0 + 9, x1 - 2, y0 + 9), fill=IDX_BG_DARK)


def draw_bottom_right(draw: ImageDraw.ImageDraw) -> None:
    x0, y0, x1, y1 = tile_rect(3, 2)
    draw_wood_frame(draw, 3, 2, False, False, True, True)
    draw_felt(draw, 3, 2, 0, 0, 4, 4)
    draw_bolt(draw, x0 + 4, y0 + 4)
    draw_bolt(draw, x1 - 5, y1 - 5)


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
    img = Image.new('P', (WIDTH, HEIGHT), IDX_BG_DARK)
    accent = Image.new('P', (TILE_SIZE, TILE_SIZE), IDX_BG_DARK)
    img.putpalette(hud.getpalette())
    accent.putpalette(hud.getpalette())
    draw = ImageDraw.Draw(img)
    accent_draw = ImageDraw.Draw(accent)

    draw_top_left(draw)
    draw_top_fill(draw, 1, False)
    draw_top_fill(draw, 2, True)
    draw_top_right(draw)

    draw_mid_left(draw)
    draw_mid_fill(draw, 1, False)
    draw_mid_fill(draw, 2, True)
    draw_mid_right(draw)

    draw_bottom_left(draw)
    draw_bottom_fill(draw, 1, False)
    draw_bottom_fill(draw, 2, True)
    draw_bottom_right(draw)
    draw_accent(accent_draw)

    img.save(repo / 'img' / 'theme.gif', save_all=False)
    accent.save(repo / 'img' / 'accent.gif', save_all=False)


if __name__ == '__main__':
    main()
