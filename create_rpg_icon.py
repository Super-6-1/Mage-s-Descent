#!/usr/bin/env python3
"""Generate 40x40 1-bit icon for Mage's Descent RPG app"""

from PIL import Image, ImageDraw
import os

def main():
    img = Image.new('1', (40, 40), 1)
    draw = ImageDraw.Draw(img)

    # Wizard hat + staff icon
    # Hat brim
    draw.line([10, 24, 30, 24], fill=0, width=2)
    # Hat body (triangle)
    draw.polygon([(14, 24), (20, 6), (26, 24)], outline=0, width=2)
    # Hat tip star
    draw.point((20, 5), fill=0)
    draw.point((19, 6), fill=0)
    draw.point((21, 6), fill=0)
    draw.point((20, 7), fill=0)

    # Staff on right side
    draw.line([30, 8, 30, 36], fill=0, width=2)
    # Staff orb
    draw.ellipse([27, 5, 33, 11], outline=0, width=1)
    draw.point((30, 8), fill=0)

    # Sword on left side
    draw.line([10, 10, 10, 36], fill=0, width=2)
    draw.line([7, 15, 13, 15], fill=0, width=1)
    draw.rectangle([8, 35, 12, 37], fill=0)

    # Small "M" at bottom
    draw.point((17, 32), fill=0)
    draw.point((18, 33), fill=0)
    draw.point((19, 32), fill=0)
    draw.point((20, 33), fill=0)
    draw.point((21, 32), fill=0)
    draw.point((17, 34), fill=0)
    draw.point((21, 34), fill=0)

    # Border
    draw.rectangle([0, 0, 39, 39], outline=0, width=1)

    # Save as raw binary (200 bytes: 40x40 / 8)
    pixels = list(img.getdata())
    data = bytearray()
    for y in range(40):
        for byte_idx in range(5):  # 40/8 = 5 bytes per row
            byte_val = 0
            for bit in range(8):
                x = byte_idx * 8 + bit
                if x < 40:
                    if pixels[y * 40 + x] == 0:
                        byte_val |= (1 << (7 - bit))
            data.append(byte_val)

    out_path = os.path.join(os.path.dirname(__file__), 'mages_descent_ICON.bin')
    with open(out_path, 'wb') as f:
        f.write(data)

    print(f"Icon saved: {out_path} ({len(data)} bytes)")

    # PNG preview
    preview_path = os.path.join(os.path.dirname(__file__), 'mages_descent_icon_preview.png')
    img.resize((160, 160), Image.NEAREST).save(preview_path)
    print(f"Preview saved: {preview_path}")


if __name__ == '__main__':
    main()
