# Mage's Descent

A classic dungeon-crawling RPG built as an OTA app for the [PocketMage PDA](https://github.com/ashtf8/PocketMage_PDA) (ESP32-S3).

Explore dungeons, fight monsters, defeat bosses, buy gear, and level up your character — all on a tiny handheld with an e-ink display and a keyboard.

## Features

- **4 Dungeons** — Crystal Caves, Goblin Warrens, Shadow Tower, and the Abyssal Sanctum (3-6 floors each with a boss at the end)
- **26 Enemies** — Including 4 dungeon bosses with unique AI (magic blasts, heavy strikes)
- **28 Items** — Weapons, armor, accessories, and consumables
- **12 Spells** — Damage, healing, buff, and debuff magic
- **12 Quests** — Kill quests and boss bounties from the quest board
- **3 Shops** — General Store, Magic Shop, and Elite Armory
- **Level cap of 30** with a full XP curve
- **Trap tiles** — Hidden traps that deal damage when stepped on
- **Background music** with 4 looping tracks (title, town, dungeon, combat) and a mute toggle
- **3 save slots** stored on SD card
- **All game data embedded in flash** — no SD card files needed (only used for saves)

## Installation

### Pre-built (easiest)

1. Download `mages_descent.tar` from [Releases](../../releases)
2. Copy it to the `/apps/` folder on your PocketMage SD card
3. Open **App Loader** on your PocketMage and install

### Build from source

**Requirements:**
- [PlatformIO](https://platformio.org/) (CLI or IDE)
- Python 3.x (for asset generation scripts)

**Steps:**

1. Clone this repo into your PocketMage project directory
2. Run the build script:
   ```
   build_rpg_ota.bat
   ```
   Or build manually with PlatformIO:
   ```
   pio run -e OTA_APP
   ```
3. The build script creates `mages_descent.tar` automatically
4. Copy the `.tar` to your SD card's `/apps/` folder and install via App Loader

## Controls

### General
| Key | Action |
|-----|--------|
| `M` | Toggle music mute |
| `Backspace` | Back / Exit |

### Town
| Key | Action |
|-----|--------|
| `1-3` | Shops (General, Magic, Armory) |
| `4` | Inn (rest and heal) |
| `5` | Enter dungeon |
| `6` / `I` | Inventory |
| `7` | Save game |
| `8` | Quest board |
| `S` | View stats |

### Dungeon
| Key | Action |
|-----|--------|
| `W/A/S/D` | Move |
| `I` | Open inventory |
| `Backspace` | Leave dungeon (at entrance) |

### Combat
| Key | Action |
|-----|--------|
| `1` | Attack |
| `2` | Defend |
| `3` | Magic |
| `4` | Item |
| `F` | Flee |

## Project Structure

```
├── Code/PocketMage_V3/src/
│   ├── APP_TEMPLATE.cpp          # Main game code (~2700 lines)
│   ├── rpg_data.h                # All game content (enemies, items, spells, dungeons, etc.)
│   └── rpg_graphics.h            # Embedded 320x240 1-bit graphics (9 images)
├── assets/
│   ├── convert_gfx_to_header.py  # Converts .bin graphics to C header
│   ├── create_rpg_graphics.py    # Generates game graphics
│   ├── create_rpg_icon.py        # Generates 40x40 app icon
│   └── mages_descent_ICON.bin    # Compiled app icon
├── build_rpg_ota.bat             # Build and package script
└── mages_descent.tar             # Pre-built OTA package
```

## Hardware

Runs on the PocketMage PDA:
- **MCU:** ESP32-S3 (16MB flash, 8MB PSRAM)
- **E-ink display:** 320x240 monochrome — maps, menus, combat screens
- **OLED display:** 256x32 — real-time status messages
- **Buzzer:** Background music and sound effects
- **Keyboard:** TCA8418-based matrix keyboard
- **SD card:** Used only for save files

## Build Stats

| Resource | Usage |
|----------|-------|
| Flash | 33.3% (872 KB / 2.6 MB) |
| RAM | 37.1% (121 KB / 320 KB) |
