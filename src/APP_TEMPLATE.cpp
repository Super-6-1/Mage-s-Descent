// ===================================================================== //
// MAGE'S DESCENT - A Classic RPG for Pocket Mage PDA                    //
// OTA APP VERSION - Game data embedded in flash, saves on SD card        //
// ===================================================================== //

#include <globals.h>
#include "esp32-hal-log.h"
#include "esp_log.h"
#include "rpg_data.h"
#include "rpg_graphics.h"

#include <Fonts/FreeMonoBold18pt8b.h>
#include <Fonts/FreeMonoBold12pt8b.h>
#include <Fonts/FreeMonoBold24pt8b.h>
#include <Fonts/FreeMono9pt8b.h>
#include <Fonts/FreeSansBold9pt8b.h>

#if OTA_APP

static constexpr const char* TAG = "RPG_OTA";

// ===================== GAME STATE DEFINITIONS =====================
enum GameState {
  GAME_TITLE,
  GAME_LOAD_SELECT,
  GAME_TOWN,
  GAME_SHOP,
  GAME_INN,
  GAME_QUEST_BOARD,
  GAME_INVENTORY,
  GAME_DUNGEON_SELECT,
  GAME_DUNGEON,
  GAME_COMBAT,
  GAME_COMBAT_MAGIC,
  GAME_COMBAT_ITEM,
  GAME_COMBAT_RESULT,
  GAME_LEVEL_UP,
  GAME_TREASURE,
  GAME_DIALOGUE,
  GAME_GAME_OVER,
  GAME_STATUS
};

enum TileType : uint8_t {
  TILE_WALL = 0,
  TILE_FLOOR = 1,
  TILE_DOOR = 2,
  TILE_STAIRS_DOWN = 3,
  TILE_STAIRS_UP = 4,
  TILE_CHEST = 5,
  TILE_NPC = 6,
  TILE_TRAP = 7,
  TILE_BOSS = 8,
  TILE_ENTRANCE = 9
};

enum ItemType : uint8_t {
  ITYPE_CONSUMABLE = 0,
  ITYPE_WEAPON = 1,
  ITYPE_ARMOR = 2,
  ITYPE_ACCESSORY = 3,
  ITYPE_KEY = 4
};

enum SpellType : uint8_t {
  STYPE_DAMAGE = 0,
  STYPE_HEAL = 1,
  STYPE_BUFF = 2,
  STYPE_DEBUFF = 3
};

enum AIType : uint8_t {
  AI_BASIC = 0,
  AI_MAGIC = 1,
  AI_DEFENSIVE = 2,
  AI_BOSS = 3
};

// ===================== DATA STRUCTURES =====================
struct Player {
  char name[16];
  uint16_t hp, maxHp;
  uint16_t mp, maxMp;
  uint16_t atk, def, mag, spd;
  uint8_t level;
  uint32_t xp, xpNext;
  uint32_t gold;
  uint8_t dungeonId;
  uint8_t posX, posY;
  uint8_t floorNum;
  uint16_t equipWeapon;
  uint16_t equipArmor;
  uint16_t equipAccessory;
  uint16_t invId[20];
  uint8_t invQty[20];
  uint8_t invCount;
  uint16_t activeQuests[8];
  uint8_t questProgress[8];
  uint8_t questCount;
  uint32_t worldFlags;
};

struct Enemy {
  uint16_t id;
  char name[20];
  uint16_t hp, maxHp;
  uint16_t atk, def, mag, spd;
  uint16_t xpReward, goldReward;
  uint16_t dropItemId;
  uint8_t dropChance;
  uint8_t aiType;
};

struct Item {
  uint16_t id;
  char name[20];
  uint8_t type;
  uint16_t value;
  int16_t stat1;
  int16_t stat2;
  char desc[48];
};

struct Spell {
  uint16_t id;
  char name[16];
  uint8_t mpCost;
  uint8_t type;
  uint16_t power;
  uint8_t unlockLevel;
};

struct DungeonInfo {
  uint16_t id;
  char name[24];
  uint8_t floors;
  uint8_t minLevel;
  uint8_t encounterRate;
  uint16_t enemyPool[8];
  uint8_t enemyPoolSize;
  uint16_t bossId;
};

struct Quest {
  uint16_t id;
  char name[24];
  char desc[64];
  uint8_t type;
  uint16_t targetId;
  uint8_t targetCount;
  uint16_t rewardGold;
  uint16_t rewardItemId;
  uint32_t rewardXp;
};

struct ShopItem {
  uint16_t itemId;
  char name[20];
  uint16_t price;
};

// ===================== GLOBAL STATE =====================
GameState gameState = GAME_TITLE;
GameState previousState = GAME_TITLE;
Player player;
Enemy currentEnemy;
DungeonInfo currentDungeon;
uint8_t dungeonMap[192]; // 16x12 tiles
bool einkNeedsRefresh = false;

// Combat state
bool playerDefending = false;
bool enemyDefending = false;
int combatTurnPhase = 0; // 0=player choose, 1=player act, 2=enemy act, 3=result
char combatMsg[64] = "";
int combatDamage = 0;
bool combatVictory = false;
uint16_t combatXpGain = 0;
uint16_t combatGoldGain = 0;
uint16_t combatDropId = 0;

// UI state
int menuCursor = 0;
int shopPage = 0;
int invPage = 0;
bool shopBuyMode = true;
ShopItem shopItems[16];
int shopItemCount = 0;
int spellPage = 0;
int questPage = 0;

// Dungeon list (scanned at startup)
struct DungeonEntry {
  uint16_t id;
  char name[24];
  uint8_t minLevel;
};
DungeonEntry dungeonList[10];
int dungeonCount = 0;

// Treasure state
uint16_t treasureItemId = 0;
uint8_t treasureQty = 0;

// Level up stat gains
int lvGainHp = 0, lvGainMp = 0, lvGainAtk = 0, lvGainDef = 0, lvGainMag = 0, lvGainSpd = 0;

// OLED message
char oledMsg[64] = "Mage's Descent";
unsigned long oledMsgTime = 0;

// Graphics buffer (shared, 320x240 1-bit = 9600 bytes)
uint8_t graphicsBuffer[9600];
bool graphicsLoaded = false;

// Save slot selection
int saveSlot = 1;

// Timing
int currentMillisKB = 0;
int currentMillisOLED = 0;

// ===================== JINGLES =====================
constexpr static const Note encounterNotes[] = {
  {NOTE_E6, 100}, {NOTE_E6, 100}, {NOTE_E6, 100}, {NOTE_G6, 200}
};
constexpr static const Note victoryNotes[] = {
  {NOTE_C6, 150}, {NOTE_E6, 150}, {NOTE_G6, 150}, {NOTE_C7, 300},
  {0, 100}, {NOTE_A6, 150}, {NOTE_C7, 300}
};
constexpr static const Note defeatNotes[] = {
  {NOTE_G5, 300}, {NOTE_F5, 300}, {NOTE_E5, 300}, {NOTE_D5, 600}
};
constexpr static const Note levelUpNotes[] = {
  {NOTE_C6, 100}, {NOTE_D6, 100}, {NOTE_E6, 100}, {NOTE_F6, 100},
  {NOTE_G6, 100}, {NOTE_A6, 100}, {NOTE_B6, 100}, {NOTE_C7, 300}
};
constexpr static const Note treasureNotes[] = {
  {NOTE_G6, 150}, {NOTE_B6, 150}, {NOTE_D7, 300}
};
constexpr static const Note shopBuyNotes[] = {
  {NOTE_E6, 100}, {NOTE_G6, 200}
};
constexpr static const Note healNotes[] = {
  {NOTE_C6, 200}, {NOTE_E6, 200}, {NOTE_G6, 200}, {NOTE_C7, 400}
};
constexpr static const Note hitNotes[] = {
  {NOTE_A5, 80}, {NOTE_E5, 80}
};

const Jingle EncounterJingle = {encounterNotes, 4};
const Jingle VictoryJingle = {victoryNotes, 7};
const Jingle DefeatJingle = {defeatNotes, 4};
const Jingle LevelUpJingle = {levelUpNotes, 8};
const Jingle TreasureJingle = {treasureNotes, 3};
const Jingle ShopBuyJingle = {shopBuyNotes, 2};
const Jingle HealJingle = {healNotes, 4};
const Jingle HitJingle = {hitNotes, 2};

// ===================== BACKGROUND MUSIC =====================

bool musicMuted = false;
int currentBgm = -1;   // -1 = none
int bgmNoteIdx = 0;
unsigned long bgmNoteStart = 0;
bool bgmPlaying = false;

// BGM track IDs
#define BGM_TITLE   0
#define BGM_TOWN    1
#define BGM_DUNGEON 2
#define BGM_COMBAT  3
#define BGM_NONE    -1

// Title theme - mysterious/inviting, 12 notes
constexpr static const Note bgmTitleNotes[] = {
  {NOTE_E5, 300}, {NOTE_G5, 300}, {NOTE_B5, 300}, {NOTE_A5, 200},
  {NOTE_G5, 200}, {NOTE_E5, 400}, {0, 200},
  {NOTE_D5, 300}, {NOTE_G5, 300}, {NOTE_A5, 300}, {NOTE_B5, 400},
  {0, 400}
};
#define BGM_TITLE_LEN 12

// Town theme - calm/pleasant, 16 notes
constexpr static const Note bgmTownNotes[] = {
  {NOTE_C5, 250}, {NOTE_E5, 250}, {NOTE_G5, 250}, {NOTE_E5, 250},
  {NOTE_A5, 300}, {NOTE_G5, 200}, {NOTE_E5, 300}, {0, 150},
  {NOTE_D5, 250}, {NOTE_F5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250},
  {NOTE_E5, 300}, {NOTE_D5, 200}, {NOTE_C5, 400}, {0, 300}
};
#define BGM_TOWN_LEN 16

// Dungeon theme - tense/dark, 12 notes
constexpr static const Note bgmDungeonNotes[] = {
  {NOTE_E4, 400}, {NOTE_G4, 200}, {NOTE_E4, 200}, {NOTE_DS4, 400},
  {0, 200}, {NOTE_D4, 300}, {NOTE_E4, 300}, {0, 200},
  {NOTE_A3, 400}, {NOTE_B3, 200}, {NOTE_E4, 500}, {0, 400}
};
#define BGM_DUNGEON_LEN 12

// Combat theme - urgent/upbeat, 16 notes
constexpr static const Note bgmCombatNotes[] = {
  {NOTE_A5, 150}, {NOTE_A5, 150}, {NOTE_E6, 200}, {NOTE_D6, 150},
  {NOTE_C6, 150}, {NOTE_A5, 200}, {0, 100},
  {NOTE_G5, 150}, {NOTE_A5, 150}, {NOTE_C6, 200}, {NOTE_D6, 150},
  {NOTE_E6, 150}, {NOTE_C6, 200}, {NOTE_A5, 200},
  {NOTE_G5, 150}, {0, 150}
};
#define BGM_COMBAT_LEN 16

const Note* bgmTracks[] = { bgmTitleNotes, bgmTownNotes, bgmDungeonNotes, bgmCombatNotes };
const int bgmLengths[] = { BGM_TITLE_LEN, BGM_TOWN_LEN, BGM_DUNGEON_LEN, BGM_COMBAT_LEN };

void setBgm(int track) {
  if (track == currentBgm) return;
  // Stop current note
  noTone(BZ_PIN);
  currentBgm = track;
  bgmNoteIdx = 0;
  bgmNoteStart = 0;
  bgmPlaying = (track >= 0);
}

void stopBgm() {
  noTone(BZ_PIN);
  bgmPlaying = false;
  currentBgm = -1;
}

int getBgmForState(GameState state); // forward declaration

void updateBgm() {
  // Auto-switch BGM based on game state
  int desiredBgm = getBgmForState(gameState);
  if (desiredBgm != currentBgm) {
    if (desiredBgm >= 0) {
      setBgm(desiredBgm);
    } else {
      stopBgm();
      return;
    }
  }

  if (!bgmPlaying || musicMuted || currentBgm < 0) {
    return;
  }

  unsigned long now = millis();

  // Start first note
  if (bgmNoteStart == 0) {
    bgmNoteStart = now;
    const Note& n = bgmTracks[currentBgm][bgmNoteIdx];
    if (n.key > 0) {
      tone(BZ_PIN, n.key);
    } else {
      noTone(BZ_PIN);
    }
    return;
  }

  // Check if current note duration has elapsed
  const Note& n = bgmTracks[currentBgm][bgmNoteIdx];
  if (now - bgmNoteStart >= (unsigned long)n.duration) {
    bgmNoteIdx++;
    if (bgmNoteIdx >= bgmLengths[currentBgm]) {
      bgmNoteIdx = 0; // Loop
    }
    bgmNoteStart = now;
    const Note& next = bgmTracks[currentBgm][bgmNoteIdx];
    if (next.key > 0) {
      tone(BZ_PIN, next.key);
    } else {
      noTone(BZ_PIN);
    }
  }
}

// Pause BGM, play a jingle, then resume BGM
void playJingleWithBgm(const Jingle& jingle) {
  noTone(BZ_PIN); // Stop current BGM note
  BZ().playJingle(jingle); // Play jingle (blocking, uses buzzer directly)
  bgmNoteStart = 0; // Reset BGM timing so it resumes cleanly
}

// Get the right BGM track for the current game state
int getBgmForState(GameState state) {
  switch (state) {
    case GAME_TITLE:
    case GAME_LOAD_SELECT:
      return BGM_TITLE;
    case GAME_TOWN:
    case GAME_SHOP:
    case GAME_INN:
    case GAME_QUEST_BOARD:
    case GAME_DUNGEON_SELECT:
    case GAME_INVENTORY:
    case GAME_STATUS:
      return BGM_TOWN;
    case GAME_DUNGEON:
    case GAME_TREASURE:
      return BGM_DUNGEON;
    case GAME_COMBAT:
    case GAME_COMBAT_MAGIC:
    case GAME_COMBAT_ITEM:
      return BGM_COMBAT;
    default:
      return BGM_NONE;
  }
}

// ===================== FILE HELPERS =====================

void sdBegin() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);
}

void sdEnd() {
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

// Load a 320x240 1-bit graphic from embedded PROGMEM data
bool loadGraphic(const char* path) {
  const uint8_t* src = nullptr;
  String p(path);
  if (p.endsWith("title.bin")) src = gfx_title;
  else if (p.endsWith("town.bin")) src = gfx_town;
  else if (p.endsWith("gameover.bin")) src = gfx_gameover;
  else if (p.endsWith("battle_1.bin")) src = gfx_battle_1;
  else if (p.endsWith("battle_2.bin")) src = gfx_battle_2;
  else if (p.endsWith("battle_3.bin")) src = gfx_battle_3;
  else if (p.endsWith("levelup.bin")) src = gfx_levelup;
  else if (p.endsWith("chest.bin")) src = gfx_chest;
  else if (p.endsWith("inn.bin")) src = gfx_inn;
  if (!src) { graphicsLoaded = false; return false; }
  memcpy_P(graphicsBuffer, src, 9600);
  graphicsLoaded = true;
  return true;
}

// ===================== PROGMEM STRING READER =====================
// Helper to read lines from a PROGMEM string (replaces SD file reading)

struct ProgmemReader {
  const char* data;
  int pos;
  int len;

  void init(const char* pgmData) {
    data = pgmData;
    pos = 0;
    len = strlen_P(pgmData);
  }

  bool available() { return pos < len; }

  String readLine() {
    String line;
    while (pos < len) {
      char c = pgm_read_byte(&data[pos++]);
      if (c == '\n') break;
      if (c != '\r') line += c;
    }
    return line;
  }
};

// Draw loaded graphic to e-ink
void drawGraphic() {
  if (!graphicsLoaded) return;
  for (int y = 0; y < 240; y++) {
    for (int x = 0; x < 320; x++) {
      int byteIdx = y * 40 + (x / 8);
      int bitIdx = 7 - (x % 8);
      if ((graphicsBuffer[byteIdx] >> bitIdx) & 1) {
        display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
  }
}

// Set OLED message with auto-clear timer
void setOledMsg(const char* msg) {
  strncpy(oledMsg, msg, 63);
  oledMsg[63] = 0;
  oledMsgTime = millis();
}

// ===================== KEY=VALUE PARSER =====================

// Parse a single key=value line. Returns true if key matches.
bool parseKV(const String& line, const char* key, String& outVal) {
  int eq = line.indexOf('=');
  if (eq < 0) return false;
  if (line.substring(0, eq) == key) {
    outVal = line.substring(eq + 1);
    outVal.trim();
    return true;
  }
  return false;
}

// ===================== CONTENT LOADERS (from embedded PROGMEM) =====================

bool loadEnemyById(uint16_t id, Enemy& out) {
  ProgmemReader r;
  r.init(DATA_ENEMIES);

  memset(&out, 0, sizeof(Enemy));
  bool inBlock = false;
  bool found = false;
  String val;

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[ENEMY]") {
      if (inBlock && out.id == id) { found = true; break; }
      inBlock = true;
      memset(&out, 0, sizeof(Enemy));
      continue;
    }
    if (!inBlock) continue;
    if (line.startsWith("[")) {
      if (out.id == id) { found = true; break; }
      inBlock = false;
      continue;
    }
    if (parseKV(line, "id", val)) out.id = val.toInt();
    else if (parseKV(line, "name", val)) strncpy(out.name, val.c_str(), 19);
    else if (parseKV(line, "hp", val)) { out.hp = val.toInt(); out.maxHp = out.hp; }
    else if (parseKV(line, "atk", val)) out.atk = val.toInt();
    else if (parseKV(line, "def", val)) out.def = val.toInt();
    else if (parseKV(line, "mag", val)) out.mag = val.toInt();
    else if (parseKV(line, "spd", val)) out.spd = val.toInt();
    else if (parseKV(line, "xp", val)) out.xpReward = val.toInt();
    else if (parseKV(line, "gold", val)) out.goldReward = val.toInt();
    else if (parseKV(line, "drop", val)) out.dropItemId = val.toInt();
    else if (parseKV(line, "dropChance", val)) out.dropChance = val.toInt();
    else if (parseKV(line, "ai", val)) out.aiType = val.toInt();
  }
  if (inBlock && out.id == id) found = true;
  return found;
}

bool loadItemById(uint16_t id, Item& out) {
  ProgmemReader r;
  r.init(DATA_ITEMS);

  memset(&out, 0, sizeof(Item));
  bool inBlock = false;
  bool found = false;
  String val;

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[ITEM]") {
      if (inBlock && out.id == id) { found = true; break; }
      inBlock = true;
      memset(&out, 0, sizeof(Item));
      continue;
    }
    if (!inBlock) continue;
    if (line.startsWith("[")) {
      if (out.id == id) { found = true; break; }
      inBlock = false;
      continue;
    }
    if (parseKV(line, "id", val)) out.id = val.toInt();
    else if (parseKV(line, "name", val)) strncpy(out.name, val.c_str(), 19);
    else if (parseKV(line, "type", val)) out.type = val.toInt();
    else if (parseKV(line, "value", val)) out.value = val.toInt();
    else if (parseKV(line, "stat1", val)) out.stat1 = val.toInt();
    else if (parseKV(line, "stat2", val)) out.stat2 = val.toInt();
    else if (parseKV(line, "desc", val)) strncpy(out.desc, val.c_str(), 47);
  }
  if (inBlock && out.id == id) found = true;
  return found;
}

bool loadSpellById(uint16_t id, Spell& out) {
  ProgmemReader r;
  r.init(DATA_SPELLS);

  memset(&out, 0, sizeof(Spell));
  bool inBlock = false;
  bool found = false;
  String val;

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[SPELL]") {
      if (inBlock && out.id == id) { found = true; break; }
      inBlock = true;
      memset(&out, 0, sizeof(Spell));
      continue;
    }
    if (!inBlock) continue;
    if (line.startsWith("[")) {
      if (out.id == id) { found = true; break; }
      inBlock = false;
      continue;
    }
    if (parseKV(line, "id", val)) out.id = val.toInt();
    else if (parseKV(line, "name", val)) strncpy(out.name, val.c_str(), 15);
    else if (parseKV(line, "mpCost", val)) out.mpCost = val.toInt();
    else if (parseKV(line, "type", val)) out.type = val.toInt();
    else if (parseKV(line, "power", val)) out.power = val.toInt();
    else if (parseKV(line, "level", val)) out.unlockLevel = val.toInt();
  }
  if (inBlock && out.id == id) found = true;
  return found;
}

// Helper to get the PROGMEM dungeon data for a given ID
const char* getDungeonData(uint16_t id) {
  switch (id) {
    case 1: return DATA_DUNGEON_1;
    case 2: return DATA_DUNGEON_2;
    case 3: return DATA_DUNGEON_3;
    case 4: return DATA_DUNGEON_4;
    default: return nullptr;
  }
}

bool loadDungeonInfo(uint16_t id, DungeonInfo& out) {
  const char* data = getDungeonData(id);
  if (!data) return false;

  ProgmemReader r;
  r.init(data);

  memset(&out, 0, sizeof(DungeonInfo));
  bool inInfo = false;
  String val;

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[INFO]") { inInfo = true; continue; }
    if (line.startsWith("[") && inInfo) break;
    if (!inInfo) continue;
    if (parseKV(line, "id", val)) out.id = val.toInt();
    else if (parseKV(line, "name", val)) strncpy(out.name, val.c_str(), 23);
    else if (parseKV(line, "floors", val)) out.floors = val.toInt();
    else if (parseKV(line, "minLevel", val)) out.minLevel = val.toInt();
    else if (parseKV(line, "encounterRate", val)) out.encounterRate = val.toInt();
    else if (parseKV(line, "enemies", val)) {
      out.enemyPoolSize = 0;
      int start = 0;
      for (int i = 0; i <= (int)val.length(); i++) {
        if (i == (int)val.length() || val[i] == ',') {
          if (out.enemyPoolSize < 8) {
            out.enemyPool[out.enemyPoolSize++] = val.substring(start, i).toInt();
          }
          start = i + 1;
        }
      }
    }
    else if (parseKV(line, "bossId", val)) out.bossId = val.toInt();
  }
  return (out.id > 0);
}

bool loadDungeonFloor(uint16_t dungeonId, uint8_t floor) {
  const char* data = getDungeonData(dungeonId);
  if (!data) return false;

  ProgmemReader r;
  r.init(data);

  char sectionName[16];
  snprintf(sectionName, sizeof(sectionName), "[FLOOR%d]", floor);
  bool inSection = false;
  int mapRow = 0;
  memset(dungeonMap, 0, 192);

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == sectionName) { inSection = true; continue; }
    if (line.startsWith("[") && inSection) break;
    if (!inSection) continue;

    if (line.startsWith("map=")) {
      String mapLine = line.substring(4);
      if (mapRow < 12) {
        for (int x = 0; x < 16 && x < (int)mapLine.length(); x++) {
          dungeonMap[mapRow * 16 + x] = mapLine[x] - '0';
        }
        mapRow++;
      }
    }
  }
  return (mapRow > 0);
}

// Get XP needed for a given level
uint32_t getXpForLevel(uint8_t level) {
  ProgmemReader r;
  r.init(DATA_LEVELCURVE);

  uint32_t xp = level * 100; // fallback
  String val;
  while (r.available()) {
    String line = r.readLine();
    line.trim();
    char key[8];
    snprintf(key, sizeof(key), "%d", level);
    if (parseKV(line, key, val)) {
      xp = val.toInt();
      break;
    }
  }
  return xp;
}

// Load shop inventory
int loadShopItems(int shopId) {
  ProgmemReader r;
  r.init(DATA_SHOPS);

  shopItemCount = 0;
  bool inBlock = false;
  int blockId = 0;
  String val;
  String itemIdStr = "";

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[SHOP]") { inBlock = true; blockId = 0; continue; }
    if (!inBlock) continue;
    if (line.startsWith("[")) {
      if (blockId == shopId && itemIdStr.length() > 0) break;
      inBlock = false; continue;
    }
    if (parseKV(line, "id", val)) blockId = val.toInt();
    else if (parseKV(line, "items", val) && blockId == shopId) itemIdStr = val;
  }

  if (itemIdStr.length() > 0) {
    int start = 0;
    for (int i = 0; i <= (int)itemIdStr.length() && shopItemCount < 16; i++) {
      if (i == (int)itemIdStr.length() || itemIdStr[i] == ',') {
        uint16_t itemId = itemIdStr.substring(start, i).toInt();
        Item item;
        if (loadItemById(itemId, item)) {
          shopItems[shopItemCount].itemId = itemId;
          strncpy(shopItems[shopItemCount].name, item.name, 19);
          shopItems[shopItemCount].price = item.value;
          shopItemCount++;
        }
        start = i + 1;
      }
    }
  }
  return shopItemCount;
}

// Better shop loader - load all items at once
void loadShopInventory(int shopId) {
  // Just use the unified loadShopItems
  loadShopItems(shopId);
}

bool loadQuestById(uint16_t id, Quest& out) {
  ProgmemReader r;
  r.init(DATA_QUESTS);

  memset(&out, 0, sizeof(Quest));
  bool inBlock = false;
  bool found = false;
  String val;

  while (r.available()) {
    String line = r.readLine();
    line.trim();
    if (line == "[QUEST]") {
      if (inBlock && out.id == id) { found = true; break; }
      inBlock = true;
      memset(&out, 0, sizeof(Quest));
      continue;
    }
    if (!inBlock) continue;
    if (line.startsWith("[")) {
      if (out.id == id) { found = true; break; }
      inBlock = false;
      continue;
    }
    if (parseKV(line, "id", val)) out.id = val.toInt();
    else if (parseKV(line, "name", val)) strncpy(out.name, val.c_str(), 23);
    else if (parseKV(line, "desc", val)) strncpy(out.desc, val.c_str(), 63);
    else if (parseKV(line, "type", val)) out.type = val.toInt();
    else if (parseKV(line, "target", val)) out.targetId = val.toInt();
    else if (parseKV(line, "count", val)) out.targetCount = val.toInt();
    else if (parseKV(line, "gold", val)) out.rewardGold = val.toInt();
    else if (parseKV(line, "item", val)) out.rewardItemId = val.toInt();
    else if (parseKV(line, "xp", val)) out.rewardXp = val.toInt();
  }
  if (inBlock && out.id == id) found = true;
  return found;
}

// ===================== SAVE / LOAD =====================

void saveGame(int slot) {
  char path[24];
  snprintf(path, sizeof(path), "/rpg/save%d.dat", slot);
  sdBegin();
  SD_MMC.remove(path);

  File f = SD_MMC.open(path, "w");
  if (!f) { sdEnd(); return; }

  f.println("[PLAYER]");
  f.println("name=" + String(player.name));
  f.println("hp=" + String(player.hp));
  f.println("maxHp=" + String(player.maxHp));
  f.println("mp=" + String(player.mp));
  f.println("maxMp=" + String(player.maxMp));
  f.println("atk=" + String(player.atk));
  f.println("def=" + String(player.def));
  f.println("mag=" + String(player.mag));
  f.println("spd=" + String(player.spd));
  f.println("level=" + String(player.level));
  f.println("xp=" + String(player.xp));
  f.println("xpNext=" + String(player.xpNext));
  f.println("gold=" + String(player.gold));
  f.println("dungeon=" + String(player.dungeonId));
  f.println("posX=" + String(player.posX));
  f.println("posY=" + String(player.posY));
  f.println("floor=" + String(player.floorNum));
  f.println("weapon=" + String(player.equipWeapon));
  f.println("armor=" + String(player.equipArmor));
  f.println("accessory=" + String(player.equipAccessory));

  f.println("[INVENTORY]");
  for (int i = 0; i < player.invCount; i++) {
    f.println(String(player.invId[i]) + "=" + String(player.invQty[i]));
  }

  f.println("[QUESTS]");
  for (int i = 0; i < player.questCount; i++) {
    f.println(String(player.activeQuests[i]) + "=" + String(player.questProgress[i]));
  }

  f.println("[FLAGS]");
  f.println("worldFlags=" + String(player.worldFlags));

  f.close();
  sdEnd();
  setOledMsg("Game Saved!");
  ESP_LOGI(TAG, "Saved to slot %d", slot);
}

bool loadGame(int slot) {
  char path[24];
  snprintf(path, sizeof(path), "/rpg/save%d.dat", slot);
  sdBegin();
  File f = SD_MMC.open(path, "r");
  if (!f) { sdEnd(); return false; }

  memset(&player, 0, sizeof(Player));
  String section = "";
  String val;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.startsWith("[")) { section = line; continue; }

    if (section == "[PLAYER]") {
      if (parseKV(line, "name", val)) strncpy(player.name, val.c_str(), 15);
      else if (parseKV(line, "hp", val)) player.hp = val.toInt();
      else if (parseKV(line, "maxHp", val)) player.maxHp = val.toInt();
      else if (parseKV(line, "mp", val)) player.mp = val.toInt();
      else if (parseKV(line, "maxMp", val)) player.maxMp = val.toInt();
      else if (parseKV(line, "atk", val)) player.atk = val.toInt();
      else if (parseKV(line, "def", val)) player.def = val.toInt();
      else if (parseKV(line, "mag", val)) player.mag = val.toInt();
      else if (parseKV(line, "spd", val)) player.spd = val.toInt();
      else if (parseKV(line, "level", val)) player.level = val.toInt();
      else if (parseKV(line, "xp", val)) player.xp = val.toInt();
      else if (parseKV(line, "xpNext", val)) player.xpNext = val.toInt();
      else if (parseKV(line, "gold", val)) player.gold = val.toInt();
      else if (parseKV(line, "dungeon", val)) player.dungeonId = val.toInt();
      else if (parseKV(line, "posX", val)) player.posX = val.toInt();
      else if (parseKV(line, "posY", val)) player.posY = val.toInt();
      else if (parseKV(line, "floor", val)) player.floorNum = val.toInt();
      else if (parseKV(line, "weapon", val)) player.equipWeapon = val.toInt();
      else if (parseKV(line, "armor", val)) player.equipArmor = val.toInt();
      else if (parseKV(line, "accessory", val)) player.equipAccessory = val.toInt();
    }
    else if (section == "[INVENTORY]") {
      int eq = line.indexOf('=');
      if (eq > 0 && player.invCount < 20) {
        player.invId[player.invCount] = line.substring(0, eq).toInt();
        player.invQty[player.invCount] = line.substring(eq + 1).toInt();
        player.invCount++;
      }
    }
    else if (section == "[QUESTS]") {
      int eq = line.indexOf('=');
      if (eq > 0 && player.questCount < 8) {
        player.activeQuests[player.questCount] = line.substring(0, eq).toInt();
        player.questProgress[player.questCount] = line.substring(eq + 1).toInt();
        player.questCount++;
      }
    }
    else if (section == "[FLAGS]") {
      if (parseKV(line, "worldFlags", val)) player.worldFlags = strtoul(val.c_str(), NULL, 10);
    }
  }
  f.close();
  sdEnd();
  setOledMsg("Game Loaded!");
  return true;
}

bool saveExists(int slot) {
  char path[24];
  snprintf(path, sizeof(path), "/rpg/save%d.dat", slot);
  sdBegin();
  bool exists = SD_MMC.exists(path);
  sdEnd();
  return exists;
}

// ===================== NEW GAME SETUP =====================

void initNewGame() {
  memset(&player, 0, sizeof(Player));
  strncpy(player.name, "Arlen", 15);
  player.hp = 30; player.maxHp = 30;
  player.mp = 10; player.maxMp = 10;
  player.atk = 5; player.def = 3;
  player.mag = 4; player.spd = 4;
  player.level = 1;
  player.xp = 0;
  player.xpNext = getXpForLevel(2);
  player.gold = 50;
  player.dungeonId = 0;
  // Start with an herb
  player.invId[0] = 1; // Herb
  player.invQty[0] = 3;
  player.invCount = 1;
}

// ===================== DUNGEON SCANNING =====================

void scanDungeonFiles() {
  dungeonCount = 0;
  for (int i = 1; i <= EMBEDDED_DUNGEON_COUNT; i++) {
    DungeonInfo info;
    if (loadDungeonInfo(i, info)) {
      dungeonList[dungeonCount].id = info.id;
      strncpy(dungeonList[dungeonCount].name, info.name, 23);
      dungeonList[dungeonCount].minLevel = info.minLevel;
      dungeonCount++;
    }
  }
  ESP_LOGI(TAG, "Found %d dungeons", dungeonCount);
}

// ===================== INVENTORY HELPERS =====================

void addItem(uint16_t itemId, uint8_t qty) {
  // Check if already in inventory
  for (int i = 0; i < player.invCount; i++) {
    if (player.invId[i] == itemId) {
      player.invQty[i] = min(99, player.invQty[i] + qty);
      return;
    }
  }
  // Add new slot
  if (player.invCount < 20) {
    player.invId[player.invCount] = itemId;
    player.invQty[player.invCount] = qty;
    player.invCount++;
  }
}

void removeItem(uint16_t itemId, uint8_t qty) {
  for (int i = 0; i < player.invCount; i++) {
    if (player.invId[i] == itemId) {
      if (player.invQty[i] <= qty) {
        // Remove slot entirely
        for (int j = i; j < player.invCount - 1; j++) {
          player.invId[j] = player.invId[j + 1];
          player.invQty[j] = player.invQty[j + 1];
        }
        player.invCount--;
      } else {
        player.invQty[i] -= qty;
      }
      return;
    }
  }
}

int getItemQty(uint16_t itemId) {
  for (int i = 0; i < player.invCount; i++) {
    if (player.invId[i] == itemId) return player.invQty[i];
  }
  return 0;
}

int getWeaponBonus() {
  if (player.equipWeapon == 0) return 0;
  Item item;
  if (loadItemById(player.equipWeapon, item)) return item.stat1;
  return 0;
}

int getArmorBonus() {
  if (player.equipArmor == 0) return 0;
  Item item;
  if (loadItemById(player.equipArmor, item)) return item.stat1;
  return 0;
}

int getAccessoryBonus() {
  if (player.equipAccessory == 0) return 0;
  Item item;
  if (loadItemById(player.equipAccessory, item)) return item.stat2;
  return 0;
}

// ===================== COMBAT HELPERS =====================

int calcPlayerDamage() {
  int bonus = getWeaponBonus();
  int dmg = player.atk + bonus - currentEnemy.def + random(-2, 3);
  return max(1, dmg);
}

int calcEnemyDamage() {
  int bonus = getArmorBonus();
  int dmg = currentEnemy.atk - player.def - bonus + random(-2, 3);
  if (playerDefending) dmg /= 2;
  return max(1, dmg);
}

int calcMagicDamage(uint16_t spellPower) {
  int dmg = spellPower + player.mag + getAccessoryBonus() - (currentEnemy.def / 2) + random(-2, 3);
  return max(1, dmg);
}

void startCombat(uint16_t enemyId) {
  if (!loadEnemyById(enemyId, currentEnemy)) {
    // Fallback enemy
    memset(&currentEnemy, 0, sizeof(Enemy));
    currentEnemy.id = 1;
    strncpy(currentEnemy.name, "Slime", 19);
    currentEnemy.hp = 10; currentEnemy.maxHp = 10;
    currentEnemy.atk = 3; currentEnemy.def = 1;
    currentEnemy.spd = 2;
    currentEnemy.xpReward = 3; currentEnemy.goldReward = 2;
  }
  playerDefending = false;
  enemyDefending = false;
  combatTurnPhase = 0;
  combatMsg[0] = 0;
  previousState = gameState;
  gameState = GAME_COMBAT;
  newState = true;
  einkNeedsRefresh = true;
  playJingleWithBgm(EncounterJingle);
  setOledMsg("Encounter!");
}

void rollEncounter() {
  if (random(100) < currentDungeon.encounterRate) {
    int idx = random(currentDungeon.enemyPoolSize);
    startCombat(currentDungeon.enemyPool[idx]);
  }
}

// Check and handle level up
bool checkLevelUp() {
  if (player.xp >= player.xpNext && player.level < 99) {
    player.level++;
    lvGainHp = 3 + random(0, 3);
    lvGainMp = 2 + random(0, 2);
    lvGainAtk = 1 + (player.level % 3 == 0 ? 1 : 0);
    lvGainDef = 1 + (player.level % 3 == 1 ? 1 : 0);
    lvGainMag = 1 + (player.level % 3 == 2 ? 1 : 0);
    lvGainSpd = (player.level % 2 == 0) ? 1 : 0;

    player.maxHp += lvGainHp;
    player.maxMp += lvGainMp;
    player.atk += lvGainAtk;
    player.def += lvGainDef;
    player.mag += lvGainMag;
    player.spd += lvGainSpd;
    player.hp = player.maxHp;
    player.mp = player.maxMp;
    player.xpNext = getXpForLevel(player.level + 1);
    return true;
  }
  return false;
}

// Check quest progress after killing an enemy
void checkQuestKill(uint16_t enemyId) {
  for (int i = 0; i < player.questCount; i++) {
    Quest q;
    if (loadQuestById(player.activeQuests[i], q)) {
      if (q.type == 0 && q.targetId == enemyId) {
        if (player.questProgress[i] < q.targetCount) {
          player.questProgress[i]++;
        }
      }
    }
  }
}

// Check if a chest flag is set
bool isChestOpened(uint8_t chestIdx) {
  return (player.worldFlags >> chestIdx) & 1;
}

void setChestOpened(uint8_t chestIdx) {
  player.worldFlags |= (1UL << chestIdx);
}

// ===================== DRAWING HELPERS =====================

void drawCentered(const char* text, int y, const GFXfont* font) {
  display.setFont(font);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((320 - w) / 2, y);
  display.print(text);
}

// Clear a rectangular area to white (for text over graphics)
void clearArea(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, GxEPD_WHITE);
}

void drawHpBar(int x, int y, int w, int h, int current, int maximum) {
  display.drawRect(x, y, w, h, GxEPD_BLACK);
  int fillW = (int)((float)current / maximum * (w - 2));
  if (fillW > 0) {
    display.fillRect(x + 1, y + 1, fillW, h - 2, GxEPD_BLACK);
  }
}

// Draw the dungeon tile map
void drawDungeonMap() {
  const int tileW = 16;
  const int tileH = 14;
  const int mapX = 4;
  const int mapY = 18;

  for (int y = 0; y < 12; y++) {
    for (int x = 0; x < 16; x++) {
      int px = mapX + x * tileW;
      int py = mapY + y * tileH;
      uint8_t tile = dungeonMap[y * 16 + x];

      switch (tile) {
        case TILE_WALL:
          display.fillRect(px, py, tileW, tileH, GxEPD_BLACK);
          break;
        case TILE_FLOOR:
          // Leave white, draw light border
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          break;
        case TILE_DOOR:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          display.drawLine(px + tileW/2, py, px + tileW/2, py + tileH - 1, GxEPD_BLACK);
          break;
        case TILE_STAIRS_DOWN:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          display.drawLine(px + 8, py + 2, px + 8, py + 10, GxEPD_BLACK);
          display.drawLine(px + 5, py + 7, px + 8, py + 10, GxEPD_BLACK);
          display.drawLine(px + 11, py + 7, px + 8, py + 10, GxEPD_BLACK);
          break;
        case TILE_STAIRS_UP:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          display.drawLine(px + 8, py + 10, px + 8, py + 2, GxEPD_BLACK);
          display.drawLine(px + 5, py + 5, px + 8, py + 2, GxEPD_BLACK);
          display.drawLine(px + 11, py + 5, px + 8, py + 2, GxEPD_BLACK);
          break;
        case TILE_CHEST:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          if (!isChestOpened(y * 16 + x)) {
            display.fillRect(px + 4, py + 4, 8, 6, GxEPD_BLACK);
          }
          break;
        case TILE_BOSS:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          display.fillCircle(px + 8, py + 7, 4, GxEPD_BLACK);
          break;
        case TILE_ENTRANCE:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          display.setFont(NULL);
          display.setCursor(px + 5, py + 4);
          display.print("E");
          break;
        default:
          display.drawRect(px, py, tileW, tileH, GxEPD_BLACK);
          break;
      }

      // Draw player
      if (x == player.posX && y == player.posY) {
        display.fillCircle(px + tileW / 2, py + tileH / 2, 4, GxEPD_BLACK);
        display.fillCircle(px + tileW / 2, py + tileH / 2, 2, GxEPD_WHITE);
      }
    }
  }
}

// ===================== OTA APP ENTRY POINTS =====================

void APP_INIT() {
  gameState = GAME_TITLE;
  memset(&player, 0, sizeof(Player));
  randomSeed(analogRead(0) + millis());

  // Ensure directories exist
  sdBegin();
  // Only need /rpg directory for save files
  if (!SD_MMC.exists("/rpg")) SD_MMC.mkdir("/rpg");
  sdEnd();

  scanDungeonFiles();
  EINK().forceSlowFullUpdate(true);
  newState = true;
  einkNeedsRefresh = true;
  playJingleWithBgm(Jingles::Startup);
  setBgm(BGM_TITLE);
  ESP_LOGI(TAG, "Mage's Descent initialized. %d dungeons found.", dungeonCount);
}

// ===================== KEYBOARD HANDLER =====================

void processKB_APP() {
  OLED().setPowerSave(false);
  currentMillisKB = millis();
  disableTimeout = false;

  // Update background music
  updateBgm();

  // OLED update
  currentMillisOLED = millis();
  if (currentMillisOLED - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillisOLED;
    if (gameState == GAME_DUNGEON || gameState == GAME_TOWN) {
      char buf[48];
      snprintf(buf, sizeof(buf), "HP:%d/%d MP:%d/%d G:%lu",
        player.hp, player.maxHp, player.mp, player.maxMp, player.gold);
      OLED().oledWord(buf);
    } else if (oledMsg[0] != 0) {
      OLED().oledWord(oledMsg);
      if (millis() - oledMsgTime > 3000) {
        // After 3 sec, show default
        char buf[48];
        snprintf(buf, sizeof(buf), "HP:%d/%d MP:%d/%d", player.hp, player.maxHp, player.mp, player.maxMp);
        strncpy(oledMsg, buf, 63);
      }
    }
  }

  if (currentMillisKB - KBBounceMillis < KB_COOLDOWN) return;

  char inchar = KB().updateKeypress();
  if (inchar == 0) return;

  // SHIFT toggle (all states)
  if (inchar == 17) {
    if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT)
      KB().setKeyboardState(NORMAL);
    else if (KB().getKeyboardState() == FUNC)
      KB().setKeyboardState(FN_SHIFT);
    else
      KB().setKeyboardState(SHIFT);
    KBBounceMillis = currentMillisKB;
    return;
  }
  // FN toggle
  if (inchar == 18) {
    if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT)
      KB().setKeyboardState(NORMAL);
    else if (KB().getKeyboardState() == SHIFT)
      KB().setKeyboardState(FN_SHIFT);
    else
      KB().setKeyboardState(FUNC);
    KBBounceMillis = currentMillisKB;
    return;
  }

  // Music mute toggle (M key - only in non-input states)
  if (inchar == 'm' || inchar == 'M') {
    if (gameState != GAME_COMBAT_MAGIC && gameState != GAME_COMBAT_ITEM &&
        gameState != GAME_INVENTORY) {
      musicMuted = !musicMuted;
      if (musicMuted) {
        noTone(BZ_PIN);
        setOledMsg("Music: OFF");
      } else {
        bgmNoteStart = 0; // Reset timing to resume
        setOledMsg("Music: ON");
      }
      KBBounceMillis = currentMillisKB;
      return;
    }
  }

  // Global exit: BACKSPACE at title = exit to PocketMage OS
  if (gameState == GAME_TITLE && (inchar == 127 || inchar == 8 || inchar == 12)) {
    stopBgm();
    setOledMsg("Exiting...");
    delay(500);
    rebootToPocketMage();
    KBBounceMillis = currentMillisKB;
    return;
  }

  switch (gameState) {

    // =================== TITLE ===================
    case GAME_TITLE:
      if (inchar == 13) { // ENTER
        gameState = GAME_LOAD_SELECT;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== LOAD SELECT ===================
    case GAME_LOAD_SELECT:
      if (inchar == 'n' || inchar == 'N') {
        initNewGame();
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
        setOledMsg("New Adventure!");
      }
      else if (inchar >= '1' && inchar <= '3') {
        int slot = inchar - '0';
        if (loadGame(slot)) {
          gameState = GAME_TOWN;
          newState = true;
          einkNeedsRefresh = true;
        } else {
          setOledMsg("No save in slot!");
        }
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TITLE;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== TOWN ===================
    case GAME_TOWN:
      if (inchar == '1') {
        loadShopInventory(1);
        shopPage = 0;
        shopBuyMode = true;
        gameState = GAME_SHOP;
        newState = true;
        einkNeedsRefresh = true;
        setOledMsg("General Store");
      }
      else if (inchar == '2') {
        loadShopInventory(2);
        shopPage = 0;
        shopBuyMode = true;
        gameState = GAME_SHOP;
        newState = true;
        einkNeedsRefresh = true;
        setOledMsg("Magic Shop");
      }
      else if (inchar == '3') {
        loadShopInventory(3);
        shopPage = 0;
        shopBuyMode = true;
        gameState = GAME_SHOP;
        newState = true;
        einkNeedsRefresh = true;
        setOledMsg("Elite Armory");
      }
      else if (inchar == '4') {
        gameState = GAME_INN;
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == '5') {
        if (dungeonCount > 0) {
          menuCursor = 0;
          gameState = GAME_DUNGEON_SELECT;
          newState = true;
          einkNeedsRefresh = true;
        } else {
          setOledMsg("No dungeons found!");
        }
      }
      else if (inchar == '6' || inchar == 'i' || inchar == 'I') {
        invPage = 0;
        previousState = GAME_TOWN;
        gameState = GAME_INVENTORY;
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == '7') {
        saveGame(1);
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == '8') {
        gameState = GAME_QUEST_BOARD;
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == 's' || inchar == 'S') {
        previousState = GAME_TOWN;
        gameState = GAME_STATUS;
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TITLE;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== SHOP ===================
    case GAME_SHOP:
      if (shopBuyMode) {
        if (inchar >= '1' && inchar <= '9') {
          int idx = shopPage * 6 + (inchar - '1');
          if (idx < shopItemCount) {
            if (player.gold >= shopItems[idx].price) {
              player.gold -= shopItems[idx].price;
              addItem(shopItems[idx].itemId, 1);
              playJingleWithBgm(ShopBuyJingle);
              char msg[48];
              snprintf(msg, sizeof(msg), "Bought %s!", shopItems[idx].name);
              setOledMsg(msg);
              einkNeedsRefresh = true;
            } else {
              setOledMsg("Not enough gold!");
            }
          }
        }
        else if (inchar == 's' || inchar == 'S') {
          shopBuyMode = false;
          invPage = 0;
          einkNeedsRefresh = true;
        }
      } else {
        // Sell mode
        if (inchar >= '1' && inchar <= '9') {
          int idx = invPage * 6 + (inchar - '1');
          if (idx < player.invCount) {
            Item item;
            if (loadItemById(player.invId[idx], item)) {
              uint16_t sellPrice = item.value / 2;
              player.gold += sellPrice;
              char msg[48];
              snprintf(msg, sizeof(msg), "Sold %s +%dg", item.name, sellPrice);
              setOledMsg(msg);
              removeItem(player.invId[idx], 1);
              einkNeedsRefresh = true;
            }
          }
        }
        else if (inchar == 'b' || inchar == 'B') {
          shopBuyMode = true;
          shopPage = 0;
          einkNeedsRefresh = true;
        }
      }
      if (inchar == 19) { // LEFT
        if (shopBuyMode && shopPage > 0) { shopPage--; einkNeedsRefresh = true; }
        else if (!shopBuyMode && invPage > 0) { invPage--; einkNeedsRefresh = true; }
      }
      else if (inchar == 21) { // RIGHT
        if (shopBuyMode && (shopPage + 1) * 6 < shopItemCount) { shopPage++; einkNeedsRefresh = true; }
        else if (!shopBuyMode && (invPage + 1) * 6 < player.invCount) { invPage++; einkNeedsRefresh = true; }
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== INN ===================
    case GAME_INN:
      if (inchar == '1' || inchar == 13) {
        int cost = player.level * 5;
        if (player.gold >= (uint32_t)cost) {
          player.gold -= cost;
          player.hp = player.maxHp;
          player.mp = player.maxMp;
          playJingleWithBgm(HealJingle);
          setOledMsg("HP & MP restored!");
          einkNeedsRefresh = true;
        } else {
          setOledMsg("Not enough gold!");
        }
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== QUEST BOARD ===================
    case GAME_QUEST_BOARD: {
      if (inchar >= '1' && inchar <= '6') {
        int questIdx = questPage * 6 + (inchar - '1');
        Quest q;
        // Try to load quest by sequential ID
        if (loadQuestById(questIdx + 1, q)) {
          // Check if already accepted
          bool alreadyHave = false;
          for (int i = 0; i < player.questCount; i++) {
            if (player.activeQuests[i] == q.id) { alreadyHave = true; break; }
          }
          if (!alreadyHave && player.questCount < 8) {
            player.activeQuests[player.questCount] = q.id;
            player.questProgress[player.questCount] = 0;
            player.questCount++;
            char msg[48];
            snprintf(msg, sizeof(msg), "Accepted: %s", q.name);
            setOledMsg(msg);
            einkNeedsRefresh = true;
          } else if (alreadyHave) {
            // Check if quest is complete
            for (int i = 0; i < player.questCount; i++) {
              if (player.activeQuests[i] == q.id && player.questProgress[i] >= q.targetCount) {
                // Complete quest
                player.gold += q.rewardGold;
                player.xp += q.rewardXp;
                if (q.rewardItemId > 0) addItem(q.rewardItemId, 1);
                // Remove quest
                for (int j = i; j < player.questCount - 1; j++) {
                  player.activeQuests[j] = player.activeQuests[j + 1];
                  player.questProgress[j] = player.questProgress[j + 1];
                }
                player.questCount--;
                char msg[48];
                snprintf(msg, sizeof(msg), "Quest done! +%dg +%luxp", q.rewardGold, q.rewardXp);
                setOledMsg(msg);
                einkNeedsRefresh = true;
                break;
              }
            }
          }
        }
      }
      else if (inchar == 19 && questPage > 0) { questPage--; einkNeedsRefresh = true; }
      else if (inchar == 21) { questPage++; einkNeedsRefresh = true; }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;
    }

    // =================== INVENTORY ===================
    case GAME_INVENTORY:
      if (inchar >= '1' && inchar <= '9') {
        int idx = invPage * 6 + (inchar - '1');
        if (idx < player.invCount) {
          Item item;
          if (loadItemById(player.invId[idx], item)) {
            if (item.type == ITYPE_CONSUMABLE) {
              // Use consumable
              if (item.stat1 > 0) player.hp = min((int)player.maxHp, player.hp + item.stat1);
              if (item.stat2 > 0) player.mp = min((int)player.maxMp, player.mp + item.stat2);
              removeItem(item.id, 1);
              char msg[48];
              snprintf(msg, sizeof(msg), "Used %s!", item.name);
              setOledMsg(msg);
              einkNeedsRefresh = true;
            }
            else if (item.type == ITYPE_WEAPON) {
              player.equipWeapon = item.id;
              char msg[48];
              snprintf(msg, sizeof(msg), "Equipped %s!", item.name);
              setOledMsg(msg);
              einkNeedsRefresh = true;
            }
            else if (item.type == ITYPE_ARMOR) {
              player.equipArmor = item.id;
              char msg[48];
              snprintf(msg, sizeof(msg), "Equipped %s!", item.name);
              setOledMsg(msg);
              einkNeedsRefresh = true;
            }
            else if (item.type == ITYPE_ACCESSORY) {
              player.equipAccessory = item.id;
              char msg[48];
              snprintf(msg, sizeof(msg), "Equipped %s!", item.name);
              setOledMsg(msg);
              einkNeedsRefresh = true;
            }
          }
        }
      }
      else if (inchar == 19 && invPage > 0) { invPage--; einkNeedsRefresh = true; }
      else if (inchar == 21 && (invPage + 1) * 6 < player.invCount) { invPage++; einkNeedsRefresh = true; }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = previousState;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== STATUS ===================
    case GAME_STATUS:
      if (inchar == 127 || inchar == 8 || inchar == 12 || inchar == 13) {
        gameState = previousState;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== DUNGEON SELECT ===================
    case GAME_DUNGEON_SELECT:
      if (inchar >= '1' && inchar <= '9') {
        int idx = inchar - '1';
        if (idx < dungeonCount) {
          if (loadDungeonInfo(dungeonList[idx].id, currentDungeon)) {
            player.dungeonId = currentDungeon.id;
            player.floorNum = 1;
            loadDungeonFloor(currentDungeon.id, 1);
            // Find entrance
            player.posX = 1; player.posY = 1;
            for (int y = 0; y < 12; y++) {
              for (int x = 0; x < 16; x++) {
                if (dungeonMap[y * 16 + x] == TILE_ENTRANCE) {
                  player.posX = x; player.posY = y;
                }
              }
            }
            gameState = GAME_DUNGEON;
            newState = true;
            einkNeedsRefresh = true;
            char msg[48];
            snprintf(msg, sizeof(msg), "Entering %s...", currentDungeon.name);
            setOledMsg(msg);
          }
        }
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== DUNGEON ===================
    case GAME_DUNGEON: {
      int newX = player.posX;
      int newY = player.posY;
      bool moved = false;

      if (inchar == 'w' || inchar == 'W') { newY--; moved = true; }
      else if (inchar == 's' || inchar == 'S') { newY++; moved = true; }
      else if (inchar == 'a' || inchar == 'A') { newX--; moved = true; }
      else if (inchar == 'd' || inchar == 'D') { newX++; moved = true; }
      else if (inchar == 'i' || inchar == 'I') {
        invPage = 0;
        previousState = GAME_DUNGEON;
        gameState = GAME_INVENTORY;
        newState = true;
        einkNeedsRefresh = true;
      }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        player.dungeonId = 0;
        gameState = GAME_TOWN;
        newState = true;
        einkNeedsRefresh = true;
        EINK().forceSlowFullUpdate(true);
        setOledMsg("Left dungeon");
      }

      if (moved && newX >= 0 && newX < 16 && newY >= 0 && newY < 12) {
        uint8_t tile = dungeonMap[newY * 16 + newX];
        if (tile != TILE_WALL) {
          player.posX = newX;
          player.posY = newY;
          einkNeedsRefresh = true;

          // Tile interactions
          if (tile == TILE_STAIRS_DOWN) {
            if (player.floorNum < currentDungeon.floors) {
              player.floorNum++;
              loadDungeonFloor(currentDungeon.id, player.floorNum);
              // Find stairs up on new floor
              for (int y = 0; y < 12; y++) {
                for (int x = 0; x < 16; x++) {
                  if (dungeonMap[y * 16 + x] == TILE_STAIRS_UP) {
                    player.posX = x; player.posY = y;
                  }
                }
              }
              char msg[32];
              snprintf(msg, sizeof(msg), "Floor %d", player.floorNum);
              setOledMsg(msg);
              EINK().forceSlowFullUpdate(true);
            }
          }
          else if (tile == TILE_STAIRS_UP) {
            if (player.floorNum > 1) {
              player.floorNum--;
              loadDungeonFloor(currentDungeon.id, player.floorNum);
              for (int y = 0; y < 12; y++) {
                for (int x = 0; x < 16; x++) {
                  if (dungeonMap[y * 16 + x] == TILE_STAIRS_DOWN) {
                    player.posX = x; player.posY = y;
                  }
                }
              }
              char msg[32];
              snprintf(msg, sizeof(msg), "Floor %d", player.floorNum);
              setOledMsg(msg);
              EINK().forceSlowFullUpdate(true);
            } else {
              // Exit dungeon
              player.dungeonId = 0;
              gameState = GAME_TOWN;
              newState = true;
              EINK().forceSlowFullUpdate(true);
              setOledMsg("Returned to town");
            }
          }
          else if (tile == TILE_CHEST) {
            uint8_t chestIdx = (player.floorNum - 1) * 16 + (newY % 12);
            if (!isChestOpened(chestIdx)) {
              setChestOpened(chestIdx);
              // Random loot
              int lootRoll = random(100);
              if (lootRoll < 40) { treasureItemId = 1; treasureQty = 1 + random(2); } // Herb
              else if (lootRoll < 60) { treasureItemId = 2; treasureQty = 1; } // Ether
              else if (lootRoll < 80) { treasureItemId = 0; treasureQty = 0; player.gold += 20 + random(30); } // Gold
              else { treasureItemId = 5; treasureQty = 1; } // Elixir
              if (treasureItemId > 0) addItem(treasureItemId, treasureQty);
              playJingleWithBgm(TreasureJingle);
              gameState = GAME_TREASURE;
              newState = true;
              einkNeedsRefresh = true;
            }
          }
          else if (tile == TILE_BOSS) {
            // Boss encounter - use bossId if set, fallback to last enemy in pool
            uint16_t bId = currentDungeon.bossId > 0 ? currentDungeon.bossId :
                           currentDungeon.enemyPool[currentDungeon.enemyPoolSize - 1];
            startCombat(bId);
          }
          else if (tile == TILE_TRAP) {
            // Hidden trap! Deal damage and convert to floor
            int trapDmg = 3 + player.floorNum * 2 + random(4);
            player.hp = max(0, (int)player.hp - trapDmg);
            dungeonMap[newY * 16 + newX] = TILE_FLOOR;
            char msg[32];
            snprintf(msg, sizeof(msg), "Trap! -%d HP!", trapDmg);
            setOledMsg(msg);
            playJingleWithBgm(HitJingle);
            einkNeedsRefresh = true;
            if (player.hp <= 0) {
              playJingleWithBgm(DefeatJingle);
              gameState = GAME_GAME_OVER;
              newState = true;
            }
          }
          else if (tile == TILE_FLOOR || tile == TILE_DOOR) {
            rollEncounter();
          }
        }
      }
      break;
    }

    // =================== COMBAT ===================
    case GAME_COMBAT:
      if (combatTurnPhase == 0) {
        if (inchar == '1') {
          // Attack
          playerDefending = false;
          int dmg = calcPlayerDamage();
          currentEnemy.hp = max(0, currentEnemy.hp - dmg);
          snprintf(combatMsg, sizeof(combatMsg), "Hit %s for %d!", currentEnemy.name, dmg);
          setOledMsg(combatMsg);
          combatTurnPhase = 2;
          einkNeedsRefresh = true;
          delay(600);
        }
        else if (inchar == '2') {
          // Defend
          playerDefending = true;
          snprintf(combatMsg, sizeof(combatMsg), "Defending!");
          setOledMsg(combatMsg);
          combatTurnPhase = 2;
          einkNeedsRefresh = true;
          delay(600);
        }
        else if (inchar == '3') {
          // Magic submenu
          spellPage = 0;
          gameState = GAME_COMBAT_MAGIC;
          newState = true;
          einkNeedsRefresh = true;
        }
        else if (inchar == '4') {
          // Item submenu
          invPage = 0;
          gameState = GAME_COMBAT_ITEM;
          newState = true;
          einkNeedsRefresh = true;
        }
        else if (inchar == '5' || inchar == 'f' || inchar == 'F') {
          // Flee
          int fleeChance = 40 + (player.spd - currentEnemy.spd) * 5;
          if (fleeChance < 15) fleeChance = 15; // Always at least 15% chance
          if (fleeChance > 90) fleeChance = 90; // Cap at 90%
          if (random(100) < fleeChance) {
            setOledMsg("Escaped!");
            delay(800);
            gameState = GAME_DUNGEON;
            newState = true;
            einkNeedsRefresh = true;
            EINK().forceSlowFullUpdate(true);
          } else {
            setOledMsg("Can't escape!");
            combatTurnPhase = 2;
            einkNeedsRefresh = true;
            delay(600);
          }
        }
      }

      // Enemy turn
      if (combatTurnPhase == 2) {
        if (currentEnemy.hp <= 0) {
          // Victory
          combatVictory = true;
          combatXpGain = currentEnemy.xpReward;
          combatGoldGain = currentEnemy.goldReward;
          combatDropId = 0;
          if (random(100) < currentEnemy.dropChance && currentEnemy.dropItemId > 0) {
            combatDropId = currentEnemy.dropItemId;
          }
          player.xp += combatXpGain;
          player.gold += combatGoldGain;
          if (combatDropId > 0) addItem(combatDropId, 1);
          checkQuestKill(currentEnemy.id);
          playJingleWithBgm(VictoryJingle);
          gameState = GAME_COMBAT_RESULT;
          newState = true;
          einkNeedsRefresh = true;
        } else {
          // Enemy attacks
          int eDmg;
          if (currentEnemy.aiType == AI_BOSS) {
            int roll = random(100);
            if (roll < 30 && currentEnemy.mag > 0) {
              // Magic blast
              eDmg = currentEnemy.mag + random(2, 6) - (player.def / 3);
              eDmg = max(2, eDmg);
              if (playerDefending) eDmg /= 2;
              snprintf(combatMsg, sizeof(combatMsg), "%s blasts! %d!", currentEnemy.name, eDmg);
            } else if (roll < 50) {
              // Heavy strike (1.5x ATK)
              eDmg = (currentEnemy.atk * 3 / 2) - player.def + random(-1, 3);
              eDmg = max(2, eDmg);
              if (playerDefending) eDmg /= 2;
              snprintf(combatMsg, sizeof(combatMsg), "%s SMASH! %d!", currentEnemy.name, eDmg);
            } else {
              eDmg = calcEnemyDamage();
              snprintf(combatMsg, sizeof(combatMsg), "%s hits! %d dmg!", currentEnemy.name, eDmg);
            }
          }
          else if (currentEnemy.aiType == AI_MAGIC && random(100) < 50) {
            eDmg = currentEnemy.mag + random(-1, 3) - (player.def / 2);
            eDmg = max(1, eDmg);
            if (playerDefending) eDmg /= 2;
            snprintf(combatMsg, sizeof(combatMsg), "%s casts! %d dmg!", currentEnemy.name, eDmg);
          }
          else if (currentEnemy.aiType == AI_DEFENSIVE && random(100) < 30) {
            enemyDefending = true;
            snprintf(combatMsg, sizeof(combatMsg), "%s defends!", currentEnemy.name);
            eDmg = 0;
          }
          else {
            eDmg = calcEnemyDamage();
            snprintf(combatMsg, sizeof(combatMsg), "%s hits! %d dmg!", currentEnemy.name, eDmg);
          }

          if (eDmg > 0) {
            player.hp = max(0, (int)player.hp - eDmg);
            playJingleWithBgm(HitJingle);
          }
          setOledMsg(combatMsg);
          delay(800);

          if (player.hp <= 0) {
            playJingleWithBgm(DefeatJingle);
            gameState = GAME_GAME_OVER;
            newState = true;
            einkNeedsRefresh = true;
          } else {
            combatTurnPhase = 0;
            playerDefending = false;
            einkNeedsRefresh = true;
          }
        }
      }
      break;

    // =================== COMBAT MAGIC ===================
    case GAME_COMBAT_MAGIC:
      if (inchar >= '1' && inchar <= '8') {
        int spellIdx = spellPage * 8 + (inchar - '0');
        Spell spell;
        if (loadSpellById(spellIdx, spell) && spell.unlockLevel <= player.level) {
          if (player.mp >= spell.mpCost) {
            player.mp -= spell.mpCost;
            if (spell.type == STYPE_DAMAGE) {
              int dmg = calcMagicDamage(spell.power);
              currentEnemy.hp = max(0, currentEnemy.hp - dmg);
              snprintf(combatMsg, sizeof(combatMsg), "%s! %d dmg!", spell.name, dmg);
            }
            else if (spell.type == STYPE_HEAL) {
              int heal = spell.power + player.mag / 2;
              player.hp = min((int)player.maxHp, player.hp + heal);
              snprintf(combatMsg, sizeof(combatMsg), "%s! +%d HP!", spell.name, heal);
            }
            else if (spell.type == STYPE_BUFF) {
              // Temporary defense buff
              player.def += spell.power;
              snprintf(combatMsg, sizeof(combatMsg), "%s! DEF+%d!", spell.name, spell.power);
            }
            else if (spell.type == STYPE_DEBUFF) {
              // Reduce enemy defense
              int reduction = min((int)spell.power, (int)currentEnemy.def);
              currentEnemy.def -= reduction;
              snprintf(combatMsg, sizeof(combatMsg), "%s! DEF-%d!", spell.name, reduction);
            }
            setOledMsg(combatMsg);
            playerDefending = false;
            combatTurnPhase = 2;
            gameState = GAME_COMBAT;
            einkNeedsRefresh = true;
            delay(600);
          } else {
            setOledMsg("Not enough MP!");
          }
        }
      }
      else if (inchar == 19 && spellPage > 0) { spellPage--; einkNeedsRefresh = true; }
      else if (inchar == 21) { spellPage++; einkNeedsRefresh = true; }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_COMBAT;
        combatTurnPhase = 0;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== COMBAT ITEM ===================
    case GAME_COMBAT_ITEM:
      if (inchar >= '1' && inchar <= '9') {
        int idx = invPage * 6 + (inchar - '1');
        if (idx < player.invCount) {
          Item item;
          if (loadItemById(player.invId[idx], item) && item.type == ITYPE_CONSUMABLE) {
            if (item.stat1 > 0) player.hp = min((int)player.maxHp, player.hp + item.stat1);
            if (item.stat2 > 0) player.mp = min((int)player.maxMp, player.mp + item.stat2);
            removeItem(item.id, 1);
            snprintf(combatMsg, sizeof(combatMsg), "Used %s!", item.name);
            setOledMsg(combatMsg);
            playerDefending = false;
            combatTurnPhase = 2;
            gameState = GAME_COMBAT;
            einkNeedsRefresh = true;
            delay(600);
          } else {
            setOledMsg("Can't use that!");
          }
        }
      }
      else if (inchar == 19 && invPage > 0) { invPage--; einkNeedsRefresh = true; }
      else if (inchar == 21 && (invPage + 1) * 6 < player.invCount) { invPage++; einkNeedsRefresh = true; }
      else if (inchar == 127 || inchar == 8 || inchar == 12) {
        gameState = GAME_COMBAT;
        combatTurnPhase = 0;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== COMBAT RESULT ===================
    case GAME_COMBAT_RESULT:
      if (inchar != 0) { // Any key
        if (checkLevelUp()) {
          playJingleWithBgm(LevelUpJingle);
          gameState = GAME_LEVEL_UP;
          newState = true;
          einkNeedsRefresh = true;
        } else {
          gameState = GAME_DUNGEON;
          newState = true;
          einkNeedsRefresh = true;
          EINK().forceSlowFullUpdate(true);
        }
      }
      break;

    // =================== LEVEL UP ===================
    case GAME_LEVEL_UP:
      if (inchar != 0) { // Any key
        gameState = GAME_DUNGEON;
        newState = true;
        einkNeedsRefresh = true;
        EINK().forceSlowFullUpdate(true);
      }
      break;

    // =================== TREASURE ===================
    case GAME_TREASURE:
      if (inchar != 0) { // Any key
        gameState = GAME_DUNGEON;
        newState = true;
        einkNeedsRefresh = true;
      }
      break;

    // =================== GAME OVER ===================
    case GAME_GAME_OVER:
      if (inchar != 0) { // Any key
        gameState = GAME_TITLE;
        newState = true;
        einkNeedsRefresh = true;
        EINK().forceSlowFullUpdate(true);
      }
      break;

    default:
      break;
  }

  KBBounceMillis = currentMillisKB;
}

// ===================== E-INK HANDLER =====================

void einkHandler_APP() {
  if (!newState && !einkNeedsRefresh) return;

  switch (gameState) {

    // =================== TITLE ===================
    case GAME_TITLE:
      if (newState) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/title.bin")) {
          drawGraphic();
        }

        // Title text (graphic leaves center clear)
        clearArea(20, 10, 280, 45);
        drawCentered("MAGE'S DESCENT", 42, &FreeMonoBold18pt8b);
        display.drawLine(40, 50, 280, 50, GxEPD_BLACK);
        clearArea(60, 180, 200, 50);
        drawCentered("A Pocket Mage RPG", 195, &FreeMono9pt8b);
        drawCentered("ENTER:Start  BKSP:Exit", 215, &FreeMono9pt8b);

        EINK().drawStatusBar("ENTER:Start <:Exit M:Mute");
        EINK().refresh();
      }
      break;

    // =================== LOAD SELECT ===================
    case GAME_LOAD_SELECT:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("SELECT GAME", 35, &FreeMonoBold12pt8b);
        display.drawLine(30, 48, 290, 48, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        display.setCursor(40, 80);
        display.print("N) New Game");

        for (int i = 1; i <= 3; i++) {
          display.setCursor(40, 80 + i * 30);
          display.print(String(i) + ") ");
          if (saveExists(i)) {
            display.print("Continue - Slot " + String(i));
          } else {
            display.print("Empty Slot " + String(i));
          }
        }

        EINK().drawStatusBar("N:New  1-3:Load  <:Back");
        EINK().refresh();
      }
      break;

    // =================== TOWN ===================
    case GAME_TOWN:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/town.bin")) {
          drawGraphic();
        }

        // Clear text areas over graphic
        clearArea(20, 8, 280, 35);
        drawCentered("THORNWALL", 30, &FreeMonoBold12pt8b);
        display.drawLine(40, 42, 280, 42, GxEPD_BLACK);

        clearArea(20, 48, 280, 96);
        display.setFont(&FreeMono9pt8b);
        display.setCursor(30, 63);  display.print("1) General");
        display.setCursor(170, 63); display.print("5) Dungeon");
        display.setCursor(30, 79);  display.print("2) Magic");
        display.setCursor(170, 79); display.print("6) Inventory");
        display.setCursor(30, 95);  display.print("3) Armory");
        display.setCursor(170, 95); display.print("7) Save");
        display.setCursor(30, 111); display.print("4) Inn");
        display.setCursor(170, 111);display.print("8) Quests");

        display.drawLine(30, 122, 290, 122, GxEPD_BLACK);

        clearArea(20, 126, 280, 60);
        display.setCursor(30, 140);
        display.print("Lv:" + String(player.level) + " HP:" + String(player.hp) + "/" + String(player.maxHp));
        display.setCursor(30, 158);
        display.print("MP:" + String(player.mp) + "/" + String(player.maxMp) + " Gold:" + String(player.gold));
        display.setCursor(30, 176);
        display.print("XP:" + String(player.xp) + "/" + String(player.xpNext));

        EINK().drawStatusBar("1-8:Select S:Stats <:Exit");
        EINK().refresh();
      }
      break;

    // =================== SHOP ===================
    case GAME_SHOP:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered(shopBuyMode ? "SHOP - BUY" : "SHOP - SELL", 28, &FreeMonoBold12pt8b);
        display.drawLine(20, 40, 300, 40, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        display.setCursor(20, 56);
        display.print("Gold: " + String(player.gold));

        if (shopBuyMode) {
          int startIdx = shopPage * 6;
          for (int i = 0; i < 6 && startIdx + i < shopItemCount; i++) {
            display.setCursor(20, 78 + i * 20);
            display.print(String(i + 1) + "." + String(shopItems[startIdx + i].name));
            display.setCursor(240, 78 + i * 20);
            display.print(String(shopItems[startIdx + i].price) + "g");
          }
        } else {
          int startIdx = invPage * 6;
          for (int i = 0; i < 6 && startIdx + i < player.invCount; i++) {
            Item item;
            if (loadItemById(player.invId[startIdx + i], item)) {
              display.setCursor(20, 78 + i * 20);
              display.print(String(i + 1) + "." + String(item.name) + " x" + String(player.invQty[startIdx + i]));
              display.setCursor(240, 78 + i * 20);
              display.print(String(item.value / 2) + "g");
            }
          }
        }

        String status = shopBuyMode ? "1-6:Buy S:Sell" : "1-6:Sell B:Buy";
        status += " </>:Pg <:Back";
        EINK().drawStatusBar(status);
        EINK().refresh();
      }
      break;

    // =================== INN ===================
    case GAME_INN:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/inn.bin")) {
          drawGraphic();
        }

        // Clear text areas
        clearArea(20, 10, 280, 42);
        drawCentered("THORNWALL INN", 35, &FreeMonoBold12pt8b);
        display.drawLine(40, 48, 280, 48, GxEPD_BLACK);

        clearArea(30, 55, 260, 110);
        display.setFont(&FreeMono9pt8b);
        display.setCursor(40, 80);
        display.print("Rest and recover?");
        display.setCursor(40, 105);
        int cost = player.level * 5;
        display.print("Cost: " + String(cost) + " gold");
        display.setCursor(40, 130);
        display.print("HP: " + String(player.hp) + "/" + String(player.maxHp));
        display.setCursor(40, 150);
        display.print("MP: " + String(player.mp) + "/" + String(player.maxMp));

        EINK().drawStatusBar("1/ENTER:Rest  <:Back");
        EINK().refresh();
      }
      break;

    // =================== QUEST BOARD ===================
    case GAME_QUEST_BOARD:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("QUEST BOARD", 28, &FreeMonoBold12pt8b);
        display.drawLine(20, 40, 300, 40, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        int yPos = 60;
        int startQ = questPage * 6 + 1;
        int endQ = startQ + 5;
        for (int i = startQ; i <= endQ; i++) {
          Quest q;
          if (loadQuestById(i, q)) {
            display.setCursor(20, yPos);
            int dispNum = i - questPage * 6;
            // Check if active
            bool active = false;
            uint8_t prog = 0;
            for (int j = 0; j < player.questCount; j++) {
              if (player.activeQuests[j] == q.id) {
                active = true;
                prog = player.questProgress[j];
                break;
              }
            }
            if (active) {
              display.print(String(dispNum) + "." + String(q.name) + " [" + String(prog) + "/" + String(q.targetCount) + "]");
            } else {
              display.print(String(dispNum) + "." + String(q.name));
            }
            yPos += 22;
          }
        }

        EINK().drawStatusBar("1-6:Accept </>:Pg <:Back");
        EINK().refresh();
      }
      break;

    // =================== INVENTORY ===================
    case GAME_INVENTORY:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("INVENTORY", 28, &FreeMonoBold12pt8b);
        display.drawLine(20, 40, 300, 40, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        // Equipment
        display.setCursor(20, 58);
        if (player.equipWeapon > 0) {
          Item w; loadItemById(player.equipWeapon, w);
          display.print("W:" + String(w.name));
        } else display.print("W: (none)");

        display.setCursor(170, 58);
        if (player.equipArmor > 0) {
          Item a; loadItemById(player.equipArmor, a);
          display.print("A:" + String(a.name));
        } else display.print("A: (none)");

        display.setCursor(20, 74);
        if (player.equipAccessory > 0) {
          Item ac; loadItemById(player.equipAccessory, ac);
          display.print("R:" + String(ac.name));
        } else display.print("R: (none)");

        display.drawLine(20, 82, 300, 82, GxEPD_BLACK);

        // Item list
        int startIdx = invPage * 6;
        for (int i = 0; i < 6 && startIdx + i < player.invCount; i++) {
          Item item;
          if (loadItemById(player.invId[startIdx + i], item)) {
            display.setCursor(20, 100 + i * 18);
            char typeCh = 'C';
            if (item.type == ITYPE_WEAPON) typeCh = 'W';
            else if (item.type == ITYPE_ARMOR) typeCh = 'A';
            else if (item.type == ITYPE_ACCESSORY) typeCh = 'R';
            display.print(String(i + 1) + ".[" + typeCh + "] " + String(item.name) + " x" + String(player.invQty[startIdx + i]));
          }
        }

        if (player.invCount == 0) {
          display.setCursor(80, 130);
          display.print("(empty)");
        }

        EINK().drawStatusBar("1-9:Use/Equip </>:Pg <:Back");
        EINK().refresh();
      }
      break;

    // =================== STATUS ===================
    case GAME_STATUS:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered(player.name, 30, &FreeMonoBold12pt8b);
        display.drawLine(30, 42, 290, 42, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        int y = 62;
        display.setCursor(30, y);  display.print("Level: " + String(player.level)); y += 20;
        display.setCursor(30, y);  display.print("HP: " + String(player.hp) + "/" + String(player.maxHp)); y += 20;
        display.setCursor(30, y);  display.print("MP: " + String(player.mp) + "/" + String(player.maxMp)); y += 20;
        display.setCursor(30, y);  display.print("ATK:" + String(player.atk) + " DEF:" + String(player.def));
        display.setCursor(190, y); display.print("MAG:" + String(player.mag)); y += 20;
        display.setCursor(30, y);  display.print("SPD:" + String(player.spd));
        display.setCursor(190, y); display.print("Gold:" + String(player.gold)); y += 20;
        display.setCursor(30, y);  display.print("XP: " + String(player.xp) + "/" + String(player.xpNext));

        EINK().drawStatusBar("ENTER/<:Back");
        EINK().refresh();
      }
      break;

    // =================== DUNGEON SELECT ===================
    case GAME_DUNGEON_SELECT:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("SELECT DUNGEON", 30, &FreeMonoBold12pt8b);
        display.drawLine(30, 42, 290, 42, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        for (int i = 0; i < dungeonCount; i++) {
          display.setCursor(30, 68 + i * 25);
          display.print(String(i + 1) + ") " + String(dungeonList[i].name) +
            " (Lv" + String(dungeonList[i].minLevel) + "+)");
        }

        if (dungeonCount == 0) {
          display.setCursor(60, 100);
          display.print("No dungeons found!");
          display.setCursor(30, 130);
          display.print("Add dungeon files to");
          display.setCursor(30, 150);
          display.print("/rpg/dungeons/ on SD");
        }

        EINK().drawStatusBar("1-9:Enter  <:Back");
        EINK().refresh();
      }
      break;

    // =================== DUNGEON ===================
    case GAME_DUNGEON:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        // Header
        display.setFont(&FreeMono9pt8b);
        display.setCursor(4, 14);
        display.print(String(currentDungeon.name) + " F" + String(player.floorNum));

        // Draw map
        drawDungeonMap();

        // Sidebar stats
        int sx = 264;
        display.setFont(&FreeMono9pt8b);
        display.setCursor(sx, 34);
        display.print("Lv" + String(player.level));
        display.setCursor(sx, 52);
        display.print("HP");
        drawHpBar(sx, 54, 50, 8, player.hp, player.maxHp);
        display.setCursor(sx, 76);
        display.print("MP");
        drawHpBar(sx, 78, 50, 8, player.mp, player.maxMp);
        display.setCursor(sx, 102);
        display.print(String(player.hp) + "/" + String(player.maxHp));
        display.setCursor(sx, 118);
        display.print(String(player.mp) + "/" + String(player.maxMp));

        EINK().drawStatusBar("WASD:Move I:Inv <:Leave");
        EINK().refresh();
      }
      break;

    // =================== COMBAT ===================
    case GAME_COMBAT:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        // Load battle background based on dungeon
        {
          char bgPath[32];
          int bgIdx = (player.dungeonId <= 3) ? player.dungeonId : 3;
          snprintf(bgPath, sizeof(bgPath), "/rpg/gfx/battle_%d.bin", bgIdx);
          if (loadGraphic(bgPath)) drawGraphic();
        }

        // Clear text areas over battle background
        clearArea(15, 5, 290, 65);
        clearArea(15, 75, 290, 50);
        clearArea(15, 125, 290, 90);

        // Enemy info at top
        drawCentered(currentEnemy.name, 25, &FreeMonoBold12pt8b);

        // Enemy HP bar
        display.setFont(&FreeMono9pt8b);
        display.setCursor(40, 45);
        display.print("HP:" + String(currentEnemy.hp) + "/" + String(currentEnemy.maxHp));
        drawHpBar(40, 50, 240, 12, currentEnemy.hp, currentEnemy.maxHp);

        display.drawLine(20, 72, 300, 72, GxEPD_BLACK);

        // Player info
        display.setCursor(20, 90);
        display.print(String(player.name) + " Lv" + String(player.level));
        display.setCursor(20, 108);
        display.print("HP:" + String(player.hp) + "/" + String(player.maxHp) +
                       "  MP:" + String(player.mp) + "/" + String(player.maxMp));

        display.drawLine(20, 118, 300, 118, GxEPD_BLACK);

        // Combat menu
        display.setCursor(30, 138);
        display.print("1) Attack    4) Item");
        display.setCursor(30, 158);
        display.print("2) Defend  F) Flee");
        display.setCursor(30, 178);
        display.print("3) Magic");

        // Last combat message
        if (combatMsg[0] != 0) {
          display.setCursor(30, 200);
          display.print(combatMsg);
        }

        EINK().drawStatusBar("1-4:Action F:Flee");
        EINK().refresh();
      }
      break;

    // =================== COMBAT MAGIC ===================
    case GAME_COMBAT_MAGIC:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("SPELLS", 28, &FreeMonoBold12pt8b);
        display.drawLine(20, 40, 300, 40, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        display.setCursor(20, 58);
        display.print("MP: " + String(player.mp) + "/" + String(player.maxMp));

        int yPos = 80;
        int startSpell = spellPage * 8 + 1;
        int endSpell = startSpell + 7;
        for (int i = startSpell; i <= endSpell; i++) {
          Spell spell;
          if (loadSpellById(i, spell) && spell.unlockLevel <= player.level) {
            display.setCursor(20, yPos);
            String typeStr = (spell.type == STYPE_DAMAGE) ? "DMG" :
                             (spell.type == STYPE_HEAL) ? "HEAL" :
                             (spell.type == STYPE_BUFF) ? "BUFF" : "DBF";
            display.print(String(i - spellPage * 8) + "." + String(spell.name) + " " + String(spell.mpCost) + "MP " + typeStr);
            yPos += 18;
          }
        }

        EINK().drawStatusBar("1-8:Cast </>:Pg <:Cancel");
        EINK().refresh();
      }
      break;

    // =================== COMBAT ITEM ===================
    case GAME_COMBAT_ITEM:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("USE ITEM", 28, &FreeMonoBold12pt8b);
        display.drawLine(20, 40, 300, 40, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        int startIdx = invPage * 6;
        int yPos = 62;
        for (int i = 0; i < 6 && startIdx + i < player.invCount; i++) {
          Item item;
          if (loadItemById(player.invId[startIdx + i], item)) {
            display.setCursor(20, yPos);
            if (item.type == ITYPE_CONSUMABLE) {
              display.print(String(i + 1) + "." + String(item.name) + " x" + String(player.invQty[startIdx + i]));
            } else {
              display.print(String(i + 1) + "." + String(item.name) + " (equip)");
            }
            yPos += 20;
          }
        }

        EINK().drawStatusBar("1-9:Use </>:Page <:Cancel");
        EINK().refresh();
      }
      break;

    // =================== COMBAT RESULT ===================
    case GAME_COMBAT_RESULT:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        drawCentered("VICTORY!", 40, &FreeMonoBold18pt8b);
        display.drawLine(40, 55, 280, 55, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        display.setCursor(50, 80);
        display.print("Defeated: " + String(currentEnemy.name));
        display.setCursor(50, 105);
        display.print("XP: +" + String(combatXpGain));
        display.setCursor(50, 125);
        display.print("Gold: +" + String(combatGoldGain));

        if (combatDropId > 0) {
          Item drop;
          if (loadItemById(combatDropId, drop)) {
            display.setCursor(50, 145);
            display.print("Found: " + String(drop.name));
          }
        }

        display.setCursor(50, 175);
        display.print("XP: " + String(player.xp) + "/" + String(player.xpNext));

        EINK().drawStatusBar("Any key:Continue");
        EINK().refresh();
      }
      break;

    // =================== LEVEL UP ===================
    case GAME_LEVEL_UP:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/levelup.bin")) drawGraphic();

        clearArea(30, 15, 260, 45);
        drawCentered("LEVEL UP!", 40, &FreeMonoBold18pt8b);
        display.drawLine(40, 55, 280, 55, GxEPD_BLACK);

        clearArea(40, 60, 240, 145);
        display.setFont(&FreeMono9pt8b);
        display.setCursor(60, 80);
        display.print("Level " + String(player.level) + "!");
        display.setCursor(60, 105);
        display.print("HP  +" + String(lvGainHp) + "  -> " + String(player.maxHp));
        display.setCursor(60, 123);
        display.print("MP  +" + String(lvGainMp) + "  -> " + String(player.maxMp));
        display.setCursor(60, 141);
        display.print("ATK +" + String(lvGainAtk) + "  -> " + String(player.atk));
        display.setCursor(60, 159);
        display.print("DEF +" + String(lvGainDef) + "  -> " + String(player.def));
        display.setCursor(60, 177);
        display.print("MAG +" + String(lvGainMag) + "  -> " + String(player.mag));
        if (lvGainSpd > 0) {
          display.setCursor(60, 195);
          display.print("SPD +" + String(lvGainSpd) + "  -> " + String(player.spd));
        }

        EINK().drawStatusBar("Any key:Continue");
        EINK().refresh();
      }
      break;

    // =================== TREASURE ===================
    case GAME_TREASURE:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/chest.bin")) drawGraphic();

        clearArea(30, 25, 260, 40);
        drawCentered("TREASURE!", 50, &FreeMonoBold18pt8b);
        display.drawLine(60, 65, 260, 65, GxEPD_BLACK);

        display.setFont(&FreeMono9pt8b);
        if (treasureItemId > 0) {
          Item item;
          if (loadItemById(treasureItemId, item)) {
            display.setCursor(80, 110);
            display.print("Found: " + String(item.name));
            display.setCursor(80, 135);
            display.print("x" + String(treasureQty));
          }
        } else {
          display.setCursor(80, 110);
          display.print("Found some gold!");
        }

        EINK().drawStatusBar("Any key:Continue");
        EINK().refresh();
      }
      break;

    // =================== GAME OVER ===================
    case GAME_GAME_OVER:
      if (newState || einkNeedsRefresh) {
        newState = false;
        einkNeedsRefresh = false;
        EINK().resetDisplay();

        if (loadGraphic("/rpg/gfx/gameover.bin")) {
          drawGraphic();
        }

        clearArea(30, 55, 260, 45);
        drawCentered("GAME OVER", 80, &FreeMonoBold24pt8b);
        clearArea(30, 110, 260, 60);
        display.setFont(&FreeMono9pt8b);
        display.setCursor(80, 130);
        display.print("Your journey ends...");
        display.setCursor(80, 160);
        display.print("Level " + String(player.level) + " | " + String(player.xp) + " XP");

        EINK().drawStatusBar("Any key:Title Screen");
        EINK().refresh();
      }
      break;

    default:
      break;
  }
}

#endif // OTA_APP
