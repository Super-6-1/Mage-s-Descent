"""Convert raw .bin graphics files to a single C header with PROGMEM arrays."""

import os

GFX_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sd_card_files", "rpg", "gfx")
OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Code", "PocketMage_V3", "src", "rpg_graphics.h")

FILES = [
    "title.bin",
    "town.bin",
    "gameover.bin",
    "battle_1.bin",
    "battle_2.bin",
    "battle_3.bin",
    "levelup.bin",
    "chest.bin",
    "inn.bin",
]

BYTES_PER_LINE = 16

def bin_to_c_array(data, name):
    lines = []
    lines.append(f"const uint8_t gfx_{name}[] PROGMEM = {{")
    for i in range(0, len(data), BYTES_PER_LINE):
        chunk = data[i:i + BYTES_PER_LINE]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        trailing = "," if i + BYTES_PER_LINE < len(data) else ""
        lines.append(f"    {hex_vals}{trailing}")
    lines.append("};")
    return "\n".join(lines)

def main():
    sections = []
    for filename in FILES:
        path = os.path.join(GFX_DIR, filename)
        with open(path, "rb") as f:
            data = f.read()
        name = os.path.splitext(filename)[0]
        print(f"Read {filename}: {len(data)} bytes")
        assert len(data) == 9600, f"{filename} is {len(data)} bytes, expected 9600"
        sections.append(bin_to_c_array(data, name))

    header = "#pragma once\n#include <pgmspace.h>\n\n" + "\n\n".join(sections) + "\n"

    os.makedirs(os.path.dirname(os.path.abspath(OUTPUT)), exist_ok=True)
    with open(OUTPUT, "w") as f:
        f.write(header)
    print(f"\nWrote {OUTPUT}")

if __name__ == "__main__":
    main()
