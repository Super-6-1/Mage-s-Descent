#!/usr/bin/env python3
"""Generate pixel art frame graphics for Mage's Descent RPG (320x240 1-bit e-ink).
   Graphics are BORDER/FRAME decorations only - center areas left white for text.
   Code uses clearArea() before drawing text, so only edges matter."""

from PIL import Image, ImageDraw, ImageFont
import os, math

WIDTH = 320
HEIGHT = 240

def draw_sprite(draw, x, y, sprite, scale=1):
    for row_idx, row in enumerate(sprite):
        for col_idx, ch in enumerate(row):
            if ch == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def draw_corner_ornament(draw, cx, cy, flip_x=False, flip_y=False):
    """Draw an RPG-style corner ornament (12x12)"""
    for i in range(12):
        x = cx - i if flip_x else cx + i
        y = cy - 0 if flip_y else cy + 0
        draw.point((x, y), fill=0)
        x2 = cx - 0 if flip_x else cx + 0
        y2 = cy - i if flip_y else cy + i
        draw.point((x2, y2), fill=0)
    # Diagonal accent
    for i in range(8):
        x = cx - i if flip_x else cx + i
        y = cy - i if flip_y else cy + i
        draw.point((x, y), fill=0)

def draw_rpg_frame(draw, x1, y1, x2, y2, thickness=2):
    """Draw a decorative double-line RPG frame"""
    for i in range(thickness):
        draw.rectangle([x1 + i, y1 + i, x2 - i, y2 - i], outline=0)
    # Corner diamonds
    sz = 5
    corners = [(x1, y1), (x2, y1), (x1, y2), (x2, y2)]
    for cx, cy in corners:
        draw.polygon([(cx - sz, cy), (cx, cy - sz), (cx + sz, cy), (cx, cy + sz)], outline=0, width=1)


# ==================== TITLE SCREEN ====================
# Text areas: y=10-55 (title), y=180-230 (instructions)
# Graphic: decorative frame + center illustration (y=60-175)
def create_title():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Outer frame
    draw.rectangle([0, 0, 319, 239], outline=0, width=2)
    draw.rectangle([3, 3, 316, 236], outline=0, width=1)

    # Corner ornaments
    draw_corner_ornament(draw, 6, 6)
    draw_corner_ornament(draw, 313, 6, flip_x=True)
    draw_corner_ornament(draw, 6, 233, flip_y=True)
    draw_corner_ornament(draw, 313, 233, flip_x=True, flip_y=True)

    # Center illustration area (y=60-175) - wizard silhouette
    wizard = [
        "....####....",
        "...######...",
        "..###..###..",
        "..########..",
        "..###..###..",
        "..##....##..",
        "..########..",
        ".####..####.",
        ".##.####.##.",
        ".##.####.##.",
        ".##.####.##.",
        "..########..",
        "...##..##...",
        "...##..##...",
        "..###..###..",
    ]
    draw_sprite(draw, 142, 68, wizard, 3)

    # Staff
    for i in range(55):
        draw.point((132, 72 + i), fill=0)
        draw.point((133, 72 + i), fill=0)
    draw.ellipse([127, 64, 139, 76], outline=0, width=2)

    # Swords flanking
    draw.line([55, 80, 55, 140], fill=0, width=2)
    draw.line([50, 95, 60, 95], fill=0, width=2)
    draw.rectangle([52, 140, 58, 146], fill=0)

    draw.line([264, 80, 264, 140], fill=0, width=2)
    draw.line([259, 95, 269, 95], fill=0, width=2)
    draw.rectangle([261, 140, 267, 146], fill=0)

    # Stars (small, only in illustration area)
    for sx, sy in [(35, 75), (285, 80), (40, 140), (280, 145), (75, 110), (245, 105)]:
        draw.line([sx - 2, sy, sx + 2, sy], fill=0)
        draw.line([sx, sy - 2, sx, sy + 2], fill=0)

    # Divider lines
    draw.line([30, 56, 290, 56], fill=0, width=1)
    draw.line([30, 175, 290, 175], fill=0, width=1)

    return img


# ==================== TOWN SCREEN ====================
# Text areas: y=8-42 (title), y=48-120 (menu), y=124-190 (stats)
# Graphic: just outer frame + subtle edge decorations
def create_town():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Outer frame
    draw.rectangle([0, 0, 319, 239], outline=0, width=2)

    # Corner brackets
    for cx, cy, dx, dy in [(5, 5, 1, 1), (314, 5, -1, 1), (5, 234, 1, -1), (314, 234, -1, -1)]:
        draw.line([cx, cy, cx + 15 * dx, cy], fill=0, width=1)
        draw.line([cx, cy, cx, cy + 15 * dy], fill=0, width=1)

    # Subtle rooftop silhouette along very top (y=0-6 only)
    for x in range(10, 310, 30):
        h = 4 + (x * 3) % 3
        draw.polygon([(x, 6), (x + 12, 6 - h), (x + 24, 6)], outline=0, width=1)

    # Bottom decorative line
    draw.line([10, 220, 310, 220], fill=0, width=1)
    for x in range(10, 310, 8):
        draw.point((x, 222), fill=0)

    return img


# ==================== GAME OVER SCREEN ====================
# Text areas: y=55-100 (GAME OVER title), y=110-170 (stats)
# Graphic: dark dithered border + skull at top
def create_gameover():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Heavy dark border (dithered edges)
    for y in range(240):
        for x in range(320):
            in_border = (x < 12 or x > 307 or y < 12 or y > 227)
            if in_border and (x + y) % 2 == 0:
                draw.point((x, y), fill=0)

    # Inner frame
    draw.rectangle([12, 12, 307, 227], outline=0, width=2)

    # Skull at top center (above text area, y=18-50)
    skull = [
        "..........",
        "..######..",
        ".########.",
        ".##.##.##.",
        ".##.##.##.",
        ".########.",
        "..#.##.#..",
        "..######..",
        "...#.#.#..",
        "..........",
    ]
    draw_sprite(draw, 140, 18, skull, 3)

    # Broken swords at bottom
    draw.line([120, 185, 135, 210], fill=0, width=2)
    draw.line([135, 210, 140, 205], fill=0, width=1)
    draw.line([185, 185, 200, 210], fill=0, width=2)
    draw.line([183, 187, 190, 183], fill=0, width=1)

    return img


# ==================== BATTLE BACKGROUNDS ====================
# Text areas: y=5-70 (enemy info), y=75-125 (player info), y=125-215 (menu+msg)
# Graphics: edge-only atmosphere decorations

def create_battle_cave():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Stalactites at very top (y=0-5 only, peek through enemy clear area edges)
    for x in range(0, 320, 12):
        h = 3 + (x * 7 + 5) % 5
        draw.polygon([(x, 0), (x + 5, h), (x + 10, 0)], fill=0)

    # Cave wall texture on left edge (x=0-12)
    for y in range(0, 240, 4):
        w = 4 + (y * 3 + 7) % 6
        for x in range(w):
            if (x + y) % 3 == 0:
                draw.point((x, y), fill=0)

    # Cave wall texture on right edge (x=308-319)
    for y in range(0, 240, 4):
        w = 4 + (y * 5 + 3) % 6
        for x in range(w):
            if (x + y) % 3 == 0:
                draw.point((319 - x, y), fill=0)

    # Rocky ground at bottom (y=218-239)
    for x in range(0, 320, 3):
        fh = 220 + (x * 11 + 5) % 18
        for y in range(fh, 240):
            if (x + y) % 3 == 0:
                draw.point((x, y), fill=0)

    # A few rocks
    draw.polygon([(5, 235), (18, 218), (30, 235)], outline=0, width=1)
    draw.polygon([(290, 235), (305, 220), (315, 235)], outline=0, width=1)

    return img


def create_battle_forest():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Tree trunks on edges only
    # Left tree
    draw.rectangle([3, 40, 10, 230], fill=0)
    draw.polygon([(0, 45), (7, 10), (14, 45)], outline=0, width=1)
    draw.polygon([(0, 30), (7, 0), (14, 30)], outline=0, width=1)

    # Right tree
    draw.rectangle([309, 50, 316, 230], fill=0)
    draw.polygon([(305, 55), (312, 20), (319, 55)], outline=0, width=1)
    draw.polygon([(305, 40), (312, 5), (319, 40)], outline=0, width=1)

    # Ground line at bottom
    draw.line([0, 225, 319, 225], fill=0, width=1)

    # Grass tufts along bottom edge
    for gx in range(5, 315, 10):
        draw.line([gx, 225, gx - 2, 220], fill=0)
        draw.line([gx, 225, gx + 1, 219], fill=0)
        draw.line([gx, 225, gx + 3, 221], fill=0)

    # Small ground dots
    for y in range(228, 240, 3):
        for x in range(0, 320, 7):
            if (x + y) % 4 == 0:
                draw.point((x, y), fill=0)

    return img


def create_battle_tower():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Stone brick pattern on left wall (x=0-12)
    for y in range(0, 240, 10):
        draw.line([0, y, 12, y], fill=0)
        offset = 0 if (y // 10) % 2 == 0 else 6
        for x in range(offset, 13, 12):
            draw.line([x, y, x, y + 10], fill=0)

    # Stone brick on right wall (x=308-319)
    for y in range(0, 240, 10):
        draw.line([308, y, 319, y], fill=0)
        offset = 0 if (y // 10) % 2 == 0 else 6
        for x in range(308 + offset, 320, 12):
            draw.line([x, y, x, y + 10], fill=0)

    # Torch on left wall
    draw.rectangle([14, 100, 18, 118], fill=0)
    draw.polygon([(12, 100), (16, 90), (20, 100)], outline=0, width=1)

    # Torch on right wall
    draw.rectangle([301, 100, 305, 118], fill=0)
    draw.polygon([(299, 100), (303, 90), (307, 100)], outline=0, width=1)

    # Floor tile pattern at bottom (y=220-239)
    draw.line([0, 220, 319, 220], fill=0, width=1)
    for x in range(0, 320, 20):
        draw.line([x, 220, x, 239], fill=0)

    return img


# ==================== LEVEL UP SCREEN ====================
# Text areas: y=15-55 (title), y=60-205 (stats)
# Graphic: radiating lines in corners + stars on edges
def create_levelup():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Decorative frame
    draw.rectangle([0, 0, 319, 239], outline=0, width=2)
    draw.rectangle([3, 3, 316, 236], outline=0, width=1)

    # Star bursts in corners only
    for cx, cy in [(15, 15), (304, 15), (15, 224), (304, 224)]:
        for angle in range(0, 360, 30):
            rad = math.radians(angle)
            x2 = cx + int(10 * math.cos(rad))
            y2 = cy + int(10 * math.sin(rad))
            draw.line([cx, cy, x2, y2], fill=0, width=1)

    # Stars along edges (not in center)
    for sx, sy in [(30, 8), (160, 5), (290, 8), (8, 120), (311, 120), (30, 232), (160, 235), (290, 232)]:
        draw.line([sx - 3, sy, sx + 3, sy], fill=0, width=1)
        draw.line([sx, sy - 3, sx, sy + 3], fill=0, width=1)
        draw.point((sx - 2, sy - 2), fill=0)
        draw.point((sx + 2, sy - 2), fill=0)
        draw.point((sx - 2, sy + 2), fill=0)
        draw.point((sx + 2, sy + 2), fill=0)

    # Up arrow on left edge as decoration
    draw.polygon([(10, 120), (5, 130), (15, 130)], fill=0)
    draw.polygon([(309, 120), (304, 130), (314, 130)], fill=0)

    return img


# ==================== TREASURE SCREEN ====================
# Text areas: y=25-65 (title), y=70-170 (item info)
# Graphic: chest illustration at bottom + sparkles on edges
def create_chest():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Frame
    draw.rectangle([0, 0, 319, 239], outline=0, width=2)

    # Sparkles on edges only
    for sx, sy in [(15, 15), (305, 15), (15, 225), (305, 225),
                    (160, 5), (160, 235), (8, 120), (312, 120)]:
        draw.line([sx - 3, sy, sx + 3, sy], fill=0)
        draw.line([sx, sy - 3, sx, sy + 3], fill=0)

    # Small chest illustration at bottom (y=180-230, centered)
    cx, cy = 140, 185
    # Chest body
    draw.rectangle([cx, cy, cx + 40, cy + 25], outline=0, width=2)
    draw.line([cx, cy + 12, cx + 40, cy + 12], fill=0)
    # Lid
    draw.rectangle([cx - 2, cy - 12, cx + 42, cy], outline=0, width=2)
    # Lock
    draw.ellipse([cx + 16, cy + 3, cx + 24, cy + 11], outline=0, width=1)

    # Glow lines from chest
    for rx in range(cx + 5, cx + 35, 6):
        draw.line([rx, cy - 12, rx + (-2 if rx < cx + 20 else 2), cy - 25], fill=0)

    return img


# ==================== INN SCREEN ====================
# Text areas: y=10-48 (title), y=55-165 (menu)
# Graphic: frame + moon/stars in corners, bed at bottom
def create_inn():
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Frame
    draw.rectangle([0, 0, 319, 239], outline=0, width=2)

    # Moon in top-right corner
    draw.ellipse([285, 8, 305, 28], outline=0, width=2)
    draw.ellipse([290, 5, 310, 25], fill=1)  # crescent cutout

    # Stars in corners
    for sx, sy in [(15, 12), (40, 20), (270, 18), (15, 225), (305, 225)]:
        draw.point((sx, sy), fill=0)
        draw.point((sx - 1, sy), fill=0)
        draw.point((sx + 1, sy), fill=0)
        draw.point((sx, sy - 1), fill=0)
        draw.point((sx, sy + 1), fill=0)

    # ZZZ along right edge
    for i, (zx, zy) in enumerate([(298, 60), (302, 50), (306, 40)]):
        sz = 4 + i
        draw.line([zx, zy, zx + sz, zy], fill=0)
        draw.line([zx + sz, zy, zx, zy + sz], fill=0)
        draw.line([zx, zy + sz, zx + sz, zy + sz], fill=0)

    # Small bed at bottom edge (y=195-225)
    bx, by = 100, 200
    draw.rectangle([bx, by, bx + 12, by + 22], outline=0, width=1)  # headboard
    draw.rectangle([bx + 12, by + 6, bx + 80, by + 22], outline=0, width=1)  # mattress
    draw.ellipse([bx + 14, by + 8, bx + 28, by + 16], outline=0)  # pillow
    draw.rectangle([bx + 12, by + 22, bx + 16, by + 26], fill=0)  # leg
    draw.rectangle([bx + 76, by + 22, bx + 80, by + 26], fill=0)  # leg

    return img


# ==================== SAVE/OUTPUT ====================
def save_as_raw_binary(img, filename):
    pixels = list(img.getdata())
    width, height = img.size
    bytes_per_row = (width + 7) // 8
    data = bytearray()
    for y in range(height):
        for byte_idx in range(bytes_per_row):
            byte_val = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                if x < width:
                    pixel_idx = y * width + x
                    if pixels[pixel_idx] == 0:
                        byte_val |= (1 << (7 - bit))
            data.append(byte_val)
    with open(filename, 'wb') as f:
        f.write(data)
    return len(data)


def main():
    out_dir = os.path.join(os.path.dirname(__file__), 'sd_card_files', 'rpg', 'gfx')
    os.makedirs(out_dir, exist_ok=True)
    preview_dir = os.path.join(os.path.dirname(__file__), 'rpg_graphics_preview')
    os.makedirs(preview_dir, exist_ok=True)

    print("Generating Mage's Descent RPG frame graphics...")
    print("=" * 50)

    graphics = [
        ("title",     "Title Screen",     create_title),
        ("town",      "Town Screen",      create_town),
        ("gameover",  "Game Over Screen",  create_gameover),
        ("battle_1",  "Battle: Cave",      create_battle_cave),
        ("battle_2",  "Battle: Forest",    create_battle_forest),
        ("battle_3",  "Battle: Tower",     create_battle_tower),
        ("levelup",   "Level Up Screen",   create_levelup),
        ("chest",     "Treasure Screen",   create_chest),
        ("inn",       "Inn Screen",        create_inn),
    ]

    for name, desc, func in graphics:
        print(f"Creating {desc}...")
        img = func()
        img.save(os.path.join(preview_dir, f"{name}.png"))
        size = save_as_raw_binary(img, os.path.join(out_dir, f"{name}.bin"))
        print(f"  {name}.bin ({size} bytes)")

    print(f"\nGenerated {len(graphics)} frame graphics")
    print(f"Binary: {out_dir}")
    print(f"Preview: {preview_dir}")


if __name__ == '__main__':
    main()
