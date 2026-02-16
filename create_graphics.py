#!/usr/bin/env python3
"""Generate pixel art graphics for Pomodoro app (320x240 1-bit e-ink display)"""

from PIL import Image, ImageDraw, ImageFont
import os

# Display dimensions
WIDTH = 320
HEIGHT = 240

def create_pixel_tomato_sprite(draw, x, y, scale=1):
    """Draw a cute pixel art tomato character (16x16 base, scalable)"""
    # 16x16 pixel tomato sprite
    sprite = [
        "....####....",
        "...######...",
        "..###..###..",
        ".####..####.",
        ".##########.",
        "############",
        "##..####..##",
        "############",
        "############",
        "############",
        ".##########.",
        ".##########.",
        "..########..",
        "...######...",
        "....####....",
        "......##....",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_clock(draw, x, y, scale=1):
    """Draw a pixel art clock/timer (16x16 base)"""
    sprite = [
        "....####....",
        "..########..",
        ".##......##.",
        ".#..####..#.",
        "#....##....#",
        "#....##....#",
        "#....##....#",
        "#....#####.#",
        "#..........#",
        "#..........#",
        ".#........#.",
        ".##......##.",
        "..########..",
        "....####....",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_star(draw, x, y, scale=1):
    """Draw a small pixel star"""
    sprite = [
        "..#..",
        "..#..",
        "#####",
        ".###.",
        ".#.#.",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_retro_border(draw, thickness=4):
    """Draw a retro game-style double border"""
    # Outer border
    draw.rectangle([0, 0, WIDTH-1, HEIGHT-1], outline=0, width=2)
    # Inner border with gap
    draw.rectangle([thickness, thickness, WIDTH-1-thickness, HEIGHT-1-thickness], outline=0, width=2)

def create_pixel_coffee(draw, x, y, scale=1):
    """Draw a pixel art coffee cup"""
    sprite = [
        "..~..~..~..",
        "...~..~....",
        "...........",
        ".#########.",
        ".#########.",
        ".####..###.",
        ".####.#.##.",
        ".####..###.",
        ".#########.",
        ".#########.",
        "..#######..",
        "...#####...",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#' or pixel == '~':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_mountain(draw, x, y, scale=1):
    """Draw a pixel art mountain scene"""
    sprite = [
        "........#........",
        ".......###.......",
        "......#####......",
        ".....##.####.....",
        "....###..#####...",
        "...####...#####..",
        "..#####....#####.",
        ".######.....#####",
        "#######......####",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_tree(draw, x, y, scale=1):
    """Draw a pixel art tree"""
    sprite = [
        "....##....",
        "...####...",
        "..######..",
        ".########.",
        "..######..",
        ".########.",
        "##########",
        "....##....",
        "....##....",
        "....##....",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_sun(draw, x, y, scale=1):
    """Draw a pixel art sun"""
    sprite = [
        "....#....#....",
        ".....#..#.....",
        "..#..####..#..",
        "...########...",
        "..##########..",
        "####......####",
        "###........###",
        "###........###",
        "####......####",
        "..##########..",
        "...########...",
        "..#..####..#..",
        ".....#..#.....",
        "....#....#....",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_pixel_book(draw, x, y, scale=1):
    """Draw a pixel art open book"""
    sprite = [
        "..##########..",
        ".#..........#.",
        "#.##......##.#",
        "#.##......##.#",
        "#.##......##.#",
        "#.##......##.#",
        "#.##......##.#",
        "#..#......#..#",
        ".#..######..#.",
        "..##########..",
    ]

    for row_idx, row in enumerate(sprite):
        for col_idx, pixel in enumerate(row):
            if pixel == '#':
                px = x + col_idx * scale
                py = y + row_idx * scale
                if scale == 1:
                    draw.point((px, py), fill=0)
                else:
                    draw.rectangle([px, py, px + scale - 1, py + scale - 1], fill=0)

def create_dither_pattern(draw, x, y, w, h, density=0.25):
    """Create a dithered rectangle pattern"""
    for py in range(y, y + h):
        for px in range(x, x + w):
            if (px + py) % int(1/density) == 0:
                draw.point((px, py), fill=0)

def create_main_menu_graphic():
    """Create the main menu background graphic"""
    img = Image.new('1', (WIDTH, HEIGHT), 1)  # 1-bit, white background
    draw = ImageDraw.Draw(img)

    # Retro double border
    create_retro_border(draw, 6)

    # Large tomato mascot in center-left
    create_pixel_tomato_sprite(draw, 30, 60, scale=6)

    # Decorative stars scattered around
    create_pixel_star(draw, 20, 20, scale=2)
    create_pixel_star(draw, 280, 30, scale=2)
    create_pixel_star(draw, 290, 180, scale=2)
    create_pixel_star(draw, 15, 200, scale=2)

    # Small clock icon near title area
    create_pixel_clock(draw, 250, 50, scale=3)

    # Decorative pixel line separators
    for i in range(0, 100, 4):
        draw.point((140 + i, 70), fill=0)
        draw.point((140 + i, 72), fill=0)

    # Bottom decorative bar
    draw.rectangle([15, HEIGHT - 25, WIDTH - 15, HEIGHT - 23], fill=0)
    draw.rectangle([15, HEIGHT - 20, WIDTH - 15, HEIGHT - 18], fill=0)

    return img

def create_quote_bg_productivity():
    """Quote background: Productivity theme (desk/coffee)"""
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Simple corner decorations
    # Top-left corner
    draw.line([(0, 0), (30, 0)], fill=0, width=2)
    draw.line([(0, 0), (0, 30)], fill=0, width=2)
    draw.line([(5, 5), (25, 5)], fill=0, width=1)
    draw.line([(5, 5), (5, 25)], fill=0, width=1)

    # Top-right corner
    draw.line([(WIDTH-30, 0), (WIDTH-1, 0)], fill=0, width=2)
    draw.line([(WIDTH-1, 0), (WIDTH-1, 30)], fill=0, width=2)
    draw.line([(WIDTH-25, 5), (WIDTH-6, 5)], fill=0, width=1)
    draw.line([(WIDTH-6, 5), (WIDTH-6, 25)], fill=0, width=1)

    # Bottom-left corner
    draw.line([(0, HEIGHT-1), (30, HEIGHT-1)], fill=0, width=2)
    draw.line([(0, HEIGHT-30), (0, HEIGHT-1)], fill=0, width=2)
    draw.line([(5, HEIGHT-6), (25, HEIGHT-6)], fill=0, width=1)
    draw.line([(5, HEIGHT-25), (5, HEIGHT-6)], fill=0, width=1)

    # Bottom-right corner
    draw.line([(WIDTH-30, HEIGHT-1), (WIDTH-1, HEIGHT-1)], fill=0, width=2)
    draw.line([(WIDTH-1, HEIGHT-30), (WIDTH-1, HEIGHT-1)], fill=0, width=2)
    draw.line([(WIDTH-25, HEIGHT-6), (WIDTH-6, HEIGHT-6)], fill=0, width=1)
    draw.line([(WIDTH-6, HEIGHT-25), (WIDTH-6, HEIGHT-6)], fill=0, width=1)

    # Coffee cup in bottom right
    create_pixel_coffee(draw, WIDTH - 50, HEIGHT - 55, scale=3)

    # Small book in bottom left
    create_pixel_book(draw, 20, HEIGHT - 50, scale=3)

    return img

def create_quote_bg_nature():
    """Quote background: Nature theme (mountains/trees/sun)"""
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Sun in top right
    create_pixel_sun(draw, WIDTH - 60, 15, scale=2)

    # Mountain range at bottom
    create_pixel_mountain(draw, 20, HEIGHT - 45, scale=4)
    create_pixel_mountain(draw, 150, HEIGHT - 35, scale=3)
    create_pixel_mountain(draw, 230, HEIGHT - 40, scale=3)

    # Trees scattered
    create_pixel_tree(draw, 15, HEIGHT - 75, scale=2)
    create_pixel_tree(draw, WIDTH - 40, HEIGHT - 70, scale=2)

    # Simple top border decoration
    for i in range(20, WIDTH - 20, 8):
        draw.rectangle([i, 8, i+4, 10], fill=0)

    return img

def create_quote_bg_motivation():
    """Quote background: Stars and energy theme"""
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Scatter stars around the edges
    star_positions = [
        (15, 15), (50, 25), (100, 10),
        (WIDTH-30, 20), (WIDTH-60, 35), (WIDTH-20, 60),
        (20, HEIGHT-40), (60, HEIGHT-25),
        (WIDTH-40, HEIGHT-35), (WIDTH-70, HEIGHT-20),
        (150, 15), (200, 25),
    ]

    for x, y in star_positions:
        create_pixel_star(draw, x, y, scale=2)

    # Decorative lines in corners
    draw.line([(10, 45), (45, 10)], fill=0, width=2)
    draw.line([(WIDTH-45, 10), (WIDTH-10, 45)], fill=0, width=2)
    draw.line([(10, HEIGHT-45), (45, HEIGHT-10)], fill=0, width=2)
    draw.line([(WIDTH-45, HEIGHT-10), (WIDTH-10, HEIGHT-45)], fill=0, width=2)

    return img

def create_quote_bg_focus():
    """Quote background: Focus/zen theme with tomato mascot"""
    img = Image.new('1', (WIDTH, HEIGHT), 1)
    draw = ImageDraw.Draw(img)

    # Small tomato mascot in corner
    create_pixel_tomato_sprite(draw, WIDTH - 55, HEIGHT - 60, scale=3)

    # Simple zen circle (ensō-like)
    # Draw a partial circle using lines
    import math
    cx, cy = 45, HEIGHT - 45
    r = 25
    for angle in range(45, 315, 5):
        rad = math.radians(angle)
        x1 = cx + int(r * math.cos(rad))
        y1 = cy + int(r * math.sin(rad))
        rad2 = math.radians(angle + 5)
        x2 = cx + int(r * math.cos(rad2))
        y2 = cy + int(r * math.sin(rad2))
        draw.line([(x1, y1), (x2, y2)], fill=0, width=2)

    # Minimal top decoration
    draw.rectangle([WIDTH//2 - 40, 10, WIDTH//2 + 40, 12], fill=0)
    draw.rectangle([WIDTH//2 - 20, 15, WIDTH//2 + 20, 16], fill=0)

    return img

def image_to_c_array(img, name):
    """Convert image to C array for embedding in code"""
    pixels = list(img.getdata())
    width, height = img.size

    # Pack pixels into bytes (8 pixels per byte, MSB first)
    bytes_per_row = (width + 7) // 8
    data = []

    for y in range(height):
        for byte_idx in range(bytes_per_row):
            byte_val = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                if x < width:
                    pixel_idx = y * width + x
                    # In 1-bit mode: 0=black, 1=white
                    # For e-ink: typically 1=black, 0=white, so invert
                    if pixels[pixel_idx] == 0:  # black pixel
                        byte_val |= (1 << (7 - bit))
            data.append(byte_val)

    # Generate C array
    c_code = f"// {name}: {width}x{height} pixels, {len(data)} bytes\n"
    c_code += f"const uint8_t {name}[] PROGMEM = {{\n"

    for i in range(0, len(data), 16):
        row = data[i:i+16]
        c_code += "  " + ", ".join(f"0x{b:02X}" for b in row) + ",\n"

    c_code = c_code.rstrip(",\n") + "\n};\n"
    c_code += f"const uint16_t {name}_width = {width};\n"
    c_code += f"const uint16_t {name}_height = {height};\n\n"

    return c_code

def save_as_raw_binary(img, filename):
    """Save image as raw binary (for SD card loading)"""
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
                    if pixels[pixel_idx] == 0:  # black pixel
                        byte_val |= (1 << (7 - bit))
            data.append(byte_val)

    with open(filename, 'wb') as f:
        f.write(data)

    return len(data)

def main():
    os.makedirs('graphics', exist_ok=True)

    print("Generating Pomodoro pixel art graphics...")
    print("=" * 50)

    # Generate main menu
    print("Creating main menu graphic...")
    menu_img = create_main_menu_graphic()
    menu_img.save('graphics/main_menu.png')
    size = save_as_raw_binary(menu_img, 'graphics/main_menu.bin')
    print(f"  Saved: graphics/main_menu.png, graphics/main_menu.bin ({size} bytes)")

    # Generate quote backgrounds
    backgrounds = [
        ("quote_bg_productivity", create_quote_bg_productivity),
        ("quote_bg_nature", create_quote_bg_nature),
        ("quote_bg_motivation", create_quote_bg_motivation),
        ("quote_bg_focus", create_quote_bg_focus),
    ]

    c_arrays = "// Auto-generated pixel art graphics for Pomodoro app\n"
    c_arrays += "// Display: 320x240 1-bit monochrome\n\n"
    c_arrays += "#include <Arduino.h>\n\n"

    for name, create_func in backgrounds:
        print(f"Creating {name}...")
        img = create_func()
        img.save(f'graphics/{name}.png')
        size = save_as_raw_binary(img, f'graphics/{name}.bin')
        c_arrays += image_to_c_array(img, name)
        print(f"  Saved: graphics/{name}.png, graphics/{name}.bin ({size} bytes)")

    # Add main menu to C arrays
    c_arrays += image_to_c_array(menu_img, "main_menu_graphic")

    # Save C header file
    with open('graphics/pomodoro_graphics.h', 'w') as f:
        f.write(c_arrays)
    print(f"\nSaved C header: graphics/pomodoro_graphics.h")

    print("\n" + "=" * 50)
    print("Graphics generation complete!")
    print("\nFiles created:")
    print("  - graphics/main_menu.png (preview)")
    print("  - graphics/main_menu.bin (raw binary)")
    print("  - graphics/quote_bg_*.png (previews)")
    print("  - graphics/quote_bg_*.bin (raw binaries)")
    print("  - graphics/pomodoro_graphics.h (C arrays for embedding)")

if __name__ == '__main__':
    main()
