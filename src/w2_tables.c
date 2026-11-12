/*
 * w2_tables.c - Warcraft II game-rules data tables. GENERATED, see w2_tables.h for provenance.
 * Numbers are game facts (default UDTA/UGRD from Mac War Data entries 0/1, Wargus scripts used as
 * documentation). No code from Wargus/Stratagus is included.
 */
#include <stddef.h>
#include "w2_tables.h"

#define N W2_NONE

const w2_unit_type w2_unit_types[W2_NUM_UNIT_TYPES] = {
  /* 0x00 Footman */
  { "Footman", "footman", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    60, 600, 0, 0, 50, {45, 45, 45, 45},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    2, 6, 3, 0, 1, 4, 10, 60, -1,
    1, 1, 6, 4, 60, 0, 0, 0,
    W2M_NONE, 1, W2A_FOOTMAN, N, 2, W2U_HUMAN_BARRACKS, N, W2U_GRUNT, 0, 0, 0, 0,
    2, 3, 4, 5, 6 },
  /* 0x01 Grunt */
  { "Grunt", "grunt", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    60, 600, 0, 0, 50, {46, 46, 46, 46},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    2, 6, 3, 0, 1, 4, 10, 60, -1,
    1, 1, 6, 4, 60, 0, 0, 0,
    W2M_NONE, 2, W2A_GRUNT, N, 3, W2U_ORC_BARRACKS, N, W2U_FOOTMAN, 0, 0, 0, 0,
    9, 10, 11, 12, 13 },
  /* 0x02 Peasant */
  { "Peasant", "peasant", 0x08080101ul, W2UX_RECT_SELECT | W2UX_COWARD | W2UX_CAN_REPAIR,
    30, 400, 0, 0, 30, {47, 47, 47, 47},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    0, 3, 2, 0, 1, 4, 10, 45, -1,
    1, 3, 6, 4, 50, 0, 0, 0,
    W2M_NONE, 1, W2A_PEASANT, N, 0, W2U_TOWN_HALL, N, W2U_PEON, 0, 0, 0, 0,
    16, 17, 18, 5, 6 },
  /* 0x03 Peon */
  { "Peon", "peon", 0x08080101ul, W2UX_RECT_SELECT | W2UX_COWARD | W2UX_CAN_REPAIR,
    30, 400, 0, 0, 30, {48, 48, 48, 48},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    0, 3, 2, 0, 1, 4, 10, 45, -1,
    1, 3, 6, 4, 50, 0, 0, 0,
    W2M_NONE, 2, W2A_PEON, N, 1, W2U_GREAT_HALL, N, W2U_PEASANT, 0, 0, 0, 0,
    9, 10, 19, 12, 13 },
  /* 0x04 Ballista -- Wargus differs: armor=5 */
  { "Ballista", "ballista", 0x00084004ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT | W2UX_EXPLODES | W2UX_GROUND_ATTACK,
    110, 900, 300, 0, 100, {49, 49, 49, 49},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 63, 63, 64, 64,
    0, 80, 0, 2, 8, 9, 5, 250, -1,
    3, 1, 11, 9, 70, 0, 0, 0,
    W2M_BALLISTA_BOLT, 0, W2A_BALLISTA, N, 16, W2U_HUMAN_BARRACKS, N, W2U_CATAPULT, 0, 0, 0, 0,
    20, 21, 4, 5, 22 },
  /* 0x05 Catapult */
  { "Catapult", "catapult", 0x00084004ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT | W2UX_EXPLODES | W2UX_GROUND_ATTACK,
    110, 900, 300, 0, 100, {50, 50, 50, 50},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 63, 63, 64, 64,
    0, 80, 0, 2, 8, 9, 5, 250, -1,
    3, 1, 11, 9, 70, 0, 0, 0,
    W2M_CATAPULT_ROCK, 0, W2A_CATAPULT, N, 17, W2U_ORC_BARRACKS, N, W2U_BALLISTA, 0, 0, 0, 0,
    20, 21, 11, 12, 22 },
  /* 0x06 Knight */
  { "Knight", "knight", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 800, 100, 0, 100, {51, 51, 51, 51},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 4, 13, 90, -1,
    1, 1, 6, 4, 63, 0, 0, 0,
    W2M_NONE, 1, W2A_KNIGHT, N, 8, W2U_HUMAN_BARRACKS, N, W2U_OGRE, 0, 0, 0, 0,
    25, 26, 27, 5, 6 },
  /* 0x07 Ogre */
  { "Ogre", "ogre", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 800, 100, 0, 100, {52, 52, 52, 52},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 4, 13, 90, -1,
    1, 1, 6, 4, 63, 0, 0, 0,
    W2M_NONE, 2, W2A_OGRE, N, 9, W2U_ORC_BARRACKS, N, W2U_KNIGHT, 0, 0, 0, 0,
    30, 31, 32, 12, 13 },
  /* 0x08 Elven Archer */
  { "Elven Archer", "archer", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    40, 500, 50, 0, 60, {53, 53, 53, 53},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    0, 3, 6, 0, 4, 5, 10, 70, -1,
    7, 1, 7, 5, 55, 0, 0, 0,
    W2M_ARROW, 1, W2A_ARCHER, N, 4, W2U_HUMAN_BARRACKS, N, W2U_AXETHROWER, 0, 0, 0, 0,
    35, 36, 37, 5, 6 },
  /* 0x09 Troll Axethrower -- Wargus differs: prio=50 */
  { "Troll Axethrower", "axethrower", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    40, 500, 50, 0, 60, {54, 54, 54, 54},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 36, 36, 72, 72,
    0, 3, 6, 0, 4, 5, 10, 70, -1,
    7, 1, 7, 5, 55, 0, 0, 0,
    W2M_AXE, 2, W2A_AXETHROWER, N, 5, W2U_ORC_BARRACKS, N, W2U_ARCHER, 0, 0, 0, 0,
    40, 41, 42, 12, 13 },
  /* 0x0A Mage */
  { "Mage", "mage", 0x0C0A0001ul, W2UX_MANA | W2UX_RECT_SELECT | W2UX_COWARD,
    60, 1200, 0, 0, 100, {55, 55, 55, 55},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    0, 0, 9, 0, 2, 9, 8, 120, -1,
    7, 1, 11, 9, 70, 0, 0, 0,
    W2M_LIGHTNING, 0, W2A_MAGE, N, 14, W2U_MAGE_TOWER, N, W2U_DEATH_KNIGHT, 0, 0, 0, 0,
    45, 46, 47, 5, 6 },
  /* 0x0B Death Knight */
  { "Death Knight", "death-knight", 0x0C0A8001ul, W2UX_MANA | W2UX_RECT_SELECT | W2UX_COWARD,
    60, 1200, 0, 0, 100, {58, 58, 58, 58},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 39, 39, 72, 72,
    0, 0, 9, 0, 3, 9, 8, 120, -1,
    7, 1, 11, 9, 70, 0, 0, 0,
    W2M_TOUCH_OF_DEATH, 0, W2A_DEATH_KNIGHT, N, 15, W2U_TEMPLE_OF_THE_DAMNED, N, W2U_MAGE, 0, 0, 0, 0,
    50, 51, 52, 12, 13 },
  /* 0x0C Paladin */
  { "Paladin", "paladin", 0x080A0001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 800, 100, 0, 110, {51, 51, 51, 51},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 5, 13, 90, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 1, W2A_PALADIN, N, 10, W2U_HUMAN_BARRACKS, N, W2U_OGRE_MAGE, 0, 0, 0, 0,
    55, 56, 57, 5, 6 },
  /* 0x0D Ogre-Mage */
  { "Ogre-Mage", "ogre-mage", 0x080A0001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 800, 100, 0, 110, {52, 52, 52, 52},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 5, 13, 90, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 2, W2A_OGRE_MAGE, N, 11, W2U_ORC_BARRACKS, N, W2U_PALADIN, 0, 0, 0, 0,
    60, 61, 62, 12, 13 },
  /* 0x0E Dwarven Demolition Squad */
  { "Dwarven Demolition Squad", "dwarves", 0x0A080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    40, 700, 250, 0, 100, {33, 33, 33, 33},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 32, 32, 56, 56,
    0, 4, 2, 0, 1, 4, 11, 200, -1,
    1, 5, 4, 2, 55, 0, 0, 0,
    W2M_NONE, 0, W2A_DWARVES, N, 12, W2U_INVENTOR, N, W2U_GOBLIN_SAPPERS, 0, 0, 0, 0,
    65, 66, 67, 5, 22 },
  /* 0x0F Goblin Sappers */
  { "Goblin Sappers", "goblin-sappers", 0x0A080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    40, 700, 250, 0, 100, {34, 34, 34, 34},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 37, 37, 56, 56,
    0, 4, 2, 0, 1, 4, 11, 200, -1,
    1, 5, 4, 2, 55, 0, 0, 0,
    W2M_NONE, 0, W2A_GOBLIN_SAPPERS, N, 13, W2U_ALCHEMIST, N, W2U_DWARVES, 0, 0, 0, 0,
    70, 71, 72, 12, 22 },
  /* 0x10 Attack Peasant -- Wargus differs: gold=0, time=0 */
  { "Attack Peasant", "attack-peasant", 0x08080001ul, W2UX_RECT_SELECT | W2UX_CAN_REPAIR,
    30, 400, 0, 0, 30, {47, 47, 47, 47},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    0, 3, 2, 0, 1, 4, 10, 45, -1,
    1, 1, 6, 4, 50, 0, 0, 0,
    W2M_NONE, 1, W2A_PEASANT, N, 0, W2U_HUMAN_BARRACKS, N, W2U_ATTACK_PEON, 0, 0, 0, 0,
    16, 17, 18, 5, 6 },
  /* 0x11 Attack Peon */
  { "Attack Peon", "attack-peon", 0x08080001ul, W2UX_RECT_SELECT | W2UX_CAN_REPAIR,
    30, 400, 0, 0, 30, {48, 48, 48, 48},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    0, 3, 2, 0, 1, 4, 10, 45, -1,
    1, 1, 6, 4, 50, 0, 0, 0,
    W2M_NONE, 2, W2A_PEON, N, 1, N, N, W2U_ATTACK_PEASANT, 0, 0, 0, 0,
    9, 10, 19, 12, 13 },
  /* 0x12 Ranger */
  { "Ranger", "ranger", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    50, 500, 50, 0, 70, {53, 53, 53, 53},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    0, 3, 6, 0, 4, 6, 10, 70, -1,
    7, 1, 9, 6, 57, 0, 0, 0,
    W2M_ARROW, 1, W2A_RANGER, N, 6, W2U_HUMAN_BARRACKS, N, W2U_BERSERKER, 0, 0, 0, 0,
    73, 36, 37, 5, 6 },
  /* 0x13 Berserker */
  { "Berserker", "berserker", 0x08080001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    50, 500, 50, 0, 70, {54, 54, 54, 54},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 36, 36, 72, 72,
    0, 3, 6, 0, 4, 6, 10, 70, -1,
    7, 1, 9, 6, 57, 0, 0, 0,
    W2M_AXE, 2, W2A_AXETHROWER, N, 7, W2U_ORC_BARRACKS, N, W2U_RANGER, 0, 0, 0, 0,
    74, 41, 42, 12, 13 },
  /* 0x14 Alleria */
  { "Alleria", "female-hero", 0x08080001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT,
    120, 500, 50, 0, 60, {53, 53, 53, 53},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    5, 10, 18, 0, 7, 9, 10, 70, -1,
    7, 1, 7, 5, 55, 0, 0, 0,
    W2M_ARROW, 1, W2A_FEMALE_HERO, N, 187, N, N, N, 0, 0, 0, 0,
    75, 36, N, 5, 6 },
  /* 0x15 Teron Gorefiend */
  { "Teron Gorefiend", "evil-knight", 0x080A8001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_MANA | W2UX_RECT_SELECT | W2UX_COWARD,
    180, 1200, 0, 0, 100, {58, 58, 58, 58},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 39, 39, 72, 72,
    2, 0, 16, 0, 4, 9, 8, 120, -1,
    7, 1, 11, 9, 70, 0, 0, 0,
    W2M_TOUCH_OF_DEATH, 2, W2A_DEATH_KNIGHT, N, 189, N, N, N, 0, 0, 0, 0,
    76, 51, N, 12, 13 },
  /* 0x16 Kurdan and Sky'ree */
  { "Kurdan and Sky'ree", "flying-angel", 0x08080002ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    250, 2500, 0, 0, 150, {35, 35, 35, 35},
    W2_RACE_HUMAN, W2_KIND_AIR, 2, 2, 63, 63, 80, 80,
    6, 0, 25, 0, 5, 9, 14, 250, -1,
    7, 1, 8, 6, 65, 0, 0, 0,
    W2M_GRIFFON_HAMMER, 0, W2A_FLYING_ANGEL, N, 191, N, N, N, 0, 0, 0, 0,
    77, 78, N, 5, 6 },
  /* 0x17 Dentarg */
  { "Dentarg", "fad-man", 0x080A0001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_MANA | W2UX_RECT_SELECT,
    300, 800, 100, 0, 100, {52, 52, 52, 52},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    8, 18, 6, 0, 1, 6, 13, 90, -1,
    1, 1, 6, 4, 63, 0, 0, 0,
    W2M_NONE, 2, W2A_OGRE, N, 194, N, N, N, 0, 0, 0, 0,
    79, 31, N, 12, 13 },
  /* 0x18 Khadgar */
  { "Khadgar", "white-mage", 0x080A0001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_MANA | W2UX_RECT_SELECT | W2UX_COWARD,
    120, 1200, 0, 0, 100, {55, 55, 55, 55},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    3, 0, 16, 0, 6, 9, 8, 120, -1,
    7, 1, 11, 9, 70, 0, 0, 0,
    W2M_LIGHTNING, 0, W2A_WHITE_MAGE, N, 193, N, N, N, 0, 0, 0, 0,
    80, 46, N, 5, 6 },
  /* 0x19 Grom Hellscream */
  { "Grom Hellscream", "beast-cry", 0x08080001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT,
    240, 600, 0, 0, 50, {46, 46, 46, 46},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    8, 16, 6, 0, 1, 5, 10, 60, -1,
    1, 1, 6, 4, 60, 0, 0, 0,
    W2M_NONE, 2, W2A_FOOTMAN, N, 190, N, N, N, 0, 0, 0, 0,
    81, 10, N, 12, 13 },
  /* 0x1A Human Oil Tanker -- Wargus differs: tw=2 */
  { "Human Oil Tanker", "human-oil-tanker", 0x00000208ul, W2UX_RECT_SELECT | W2UX_COWARD,
    90, 400, 200, 0, 40, {59, 59, 59, 59},
    W2_RACE_HUMAN, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    10, 0, 0, 0, 1, 4, 10, 50, -1,
    0, 4, 0, 0, 50, 10, 0, 0,
    W2M_NONE, 3, W2A_HUMAN_OIL_TANKER, N, 18, W2U_HUMAN_SHIPYARD, N, W2U_ORC_OIL_TANKER, 0, 0, 0, 0,
    84, 85, 86, 5, 87 },
  /* 0x1B Orc Oil Tanker -- Wargus differs: tw=2 */
  { "Orc Oil Tanker", "orc-oil-tanker", 0x00000208ul, W2UX_RECT_SELECT | W2UX_COWARD,
    90, 400, 200, 0, 40, {60, 60, 60, 60},
    W2_RACE_ORC, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    10, 0, 0, 0, 1, 4, 10, 50, -1,
    0, 4, 0, 0, 50, 10, 0, 0,
    W2M_NONE, 3, W2A_ORC_OIL_TANKER, N, 19, W2U_ORC_SHIPYARD, N, W2U_HUMAN_OIL_TANKER, 0, 0, 0, 0,
    90, 85, 91, 12, 87 },
  /* 0x1C Human Transport -- Wargus differs: tw=2 */
  { "Human Transport", "human-transport", 0x00000408ul, W2UX_ARMOR_UPG | W2UX_RECT_SELECT | W2UX_TRANSPORT,
    150, 600, 200, 500, 50, {39, 39, 39, 39},
    W2_RACE_HUMAN, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    0, 0, 0, 0, 1, 4, 10, 70, -1,
    0, 6, 0, 0, 70, 15, 0, 0,
    W2M_NONE, 3, W2A_HUMAN_TRANSPORT, N, 20, W2U_HUMAN_SHIPYARD, N, W2U_ORC_TRANSPORT, 0, 0, 0, 6,
    84, 92, 86, 5, 87 },
  /* 0x1D Orc Transport -- Wargus differs: tw=2 */
  { "Orc Transport", "orc-transport", 0x00000408ul, W2UX_ARMOR_UPG | W2UX_RECT_SELECT | W2UX_TRANSPORT,
    150, 600, 200, 500, 50, {40, 40, 40, 40},
    W2_RACE_ORC, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    0, 0, 0, 0, 1, 4, 10, 70, -1,
    0, 6, 0, 0, 70, 15, 0, 0,
    W2M_NONE, 3, W2A_ORC_TRANSPORT, N, 21, W2U_ORC_SHIPYARD, N, W2U_HUMAN_TRANSPORT, 0, 0, 0, 6,
    90, 93, 91, 12, 87 },
  /* 0x1E Elven Destroyer -- Wargus differs: tw=2 */
  { "Elven Destroyer", "human-destroyer", 0x00080008ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    100, 700, 350, 700, 150, {61, 61, 61, 61},
    W2_RACE_HUMAN, W2_KIND_SEA, 1, 1, 63, 63, 80, 88,
    10, 35, 0, 0, 4, 8, 10, 90, -1,
    7, 1, 10, 8, 65, 20, 0, 0,
    W2M_SMALL_CANNON, 3, W2A_ELVEN_DESTROYER, N, 22, W2U_HUMAN_SHIPYARD, N, W2U_ORC_DESTROYER, 0, 0, 0, 0,
    84, 92, 86, 5, 87 },
  /* 0x1F Troll Destroyer -- Wargus differs: tw=2 */
  { "Troll Destroyer", "orc-destroyer", 0x00080008ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    100, 700, 350, 700, 150, {62, 62, 62, 62},
    W2_RACE_ORC, W2_KIND_SEA, 1, 1, 63, 63, 88, 88,
    10, 35, 0, 0, 4, 8, 10, 90, -1,
    7, 1, 10, 8, 65, 20, 0, 0,
    W2M_SMALL_CANNON, 3, W2A_TROLL_DESTROYER, N, 23, W2U_ORC_SHIPYARD, N, W2U_HUMAN_DESTROYER, 0, 0, 0, 0,
    90, 93, 91, 12, 87 },
  /* 0x20 Battleship -- Wargus differs: tw=2 */
  { "Battleship", "battleship", 0x00084008ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT | W2UX_GROUND_ATTACK,
    150, 1000, 500, 1000, 300, {41, 41, 41, 41},
    W2_RACE_HUMAN, W2_KIND_SEA, 1, 1, 70, 70, 88, 88,
    15, 130, 0, 0, 6, 8, 6, 140, -1,
    3, 1, 10, 8, 63, 25, 0, 0,
    W2M_BIG_CANNON, 3, W2A_BATTLESHIP, N, 24, W2U_HUMAN_SHIPYARD, N, W2U_OGRE_JUGGERNAUGHT, 0, 0, 0, 0,
    84, 92, 86, 5, 87 },
  /* 0x21 Ogre Juggernaught -- Wargus differs: tw=2 */
  { "Ogre Juggernaught", "ogre-juggernaught", 0x00084008ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT | W2UX_GROUND_ATTACK,
    150, 1000, 500, 1000, 300, {42, 42, 42, 42},
    W2_RACE_ORC, W2_KIND_SEA, 1, 1, 70, 70, 88, 88,
    15, 130, 0, 0, 6, 8, 6, 140, -1,
    3, 1, 10, 8, 63, 25, 0, 0,
    W2M_BIG_CANNON, 3, W2A_BATTLESHIP, N, 25, W2U_ORC_SHIPYARD, N, W2U_BATTLESHIP, 0, 0, 0, 0,
    90, 93, 91, 12, 87 },
  /* 0x22 (unused) */
  { "", "", 0x00000000ul, W2UX_UNUSED,
    0, 0, 0, 0, 0, {0, 0, 0, 0},
    W2_RACE_NEUTRAL, W2_KIND_NONE, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, N, N, N, N, N, N, 0, 0, 0, 0,
    N, N, N, N, N },
  /* 0x23 Deathwing */
  { "Deathwing", "fire-breeze", 0x08080002ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    800, 2500, 0, 0, 150, {36, 36, 36, 36},
    W2_RACE_ORC, W2_KIND_AIR, 2, 2, 71, 71, 88, 80,
    10, 10, 25, 0, 5, 9, 14, 250, -1,
    7, 1, 8, 6, 65, 0, 0, 0,
    W2M_DRAGON_BREATH, 0, W2A_FIRE_BREEZE, N, 192, N, N, N, 0, 0, 0, 0,
    94, 95, N, 12, 22 },
  /* 0x24 (unused) */
  { "", "", 0x00000008ul, W2UX_UNUSED | W2UX_RECT_SELECT,
    60, 400, 0, 0, 0, {0, 0, 0, 0},
    W2_RACE_NEUTRAL, W2_KIND_SEA, 1, 1, 63, 63, 0, 0,
    2, 9, 1, 0, 1, 4, 0, 60, 0,
    0, 1, 20, 10, 40, 0, 0, 0,
    W2M_NONE, 0, N, N, N, N, N, N, 0, 0, 0, 0,
    N, N, N, N, N },
  /* 0x25 (unused) */
  { "", "", 0x00000008ul, W2UX_UNUSED | W2UX_RECT_SELECT,
    60, 400, 0, 0, 0, {0, 0, 0, 0},
    W2_RACE_NEUTRAL, W2_KIND_SEA, 1, 1, 63, 63, 0, 0,
    2, 9, 1, 0, 1, 4, 0, 60, 0,
    0, 1, 20, 10, 40, 0, 0, 0,
    W2M_NONE, 0, N, N, N, N, N, N, 0, 0, 0, 0,
    N, N, N, N, N },
  /* 0x26 Gnomish Submarine -- Wargus differs: tw=2 */
  { "Gnomish Submarine", "human-submarine", 0x000800C8ul, W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    60, 800, 150, 900, 120, {43, 43, 182, 182},
    W2_RACE_HUMAN, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    0, 50, 0, 0, 4, 5, 7, 100, -1,
    2, 1, 7, 5, 60, 20, 0, 0,
    W2M_SUBMARINE_MISSILE, 3, W2A_GNOMISH_SUBMARINE, N, 26, W2U_HUMAN_SHIPYARD, N, W2U_ORC_SUBMARINE, 0, 0, 0, 0,
    97, 92, 86, 5, 87 },
  /* 0x27 Giant Turtle -- Wargus differs: tw=2 */
  { "Giant Turtle", "orc-submarine", 0x000800C8ul, W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    60, 800, 150, 900, 120, {44, 44, 183, 183},
    W2_RACE_ORC, W2_KIND_SEA, 1, 1, 63, 63, 72, 72,
    0, 50, 0, 0, 4, 5, 7, 100, -1,
    2, 1, 7, 5, 60, 20, 0, 0,
    W2M_TURTLE_MISSILE, 3, W2A_ORC_SUBMARINE, N, 27, W2U_ORC_SHIPYARD, N, W2U_HUMAN_SUBMARINE, 0, 0, 0, 0,
    90, 93, 91, 12, 87 },
  /* 0x28 Gnomish Flying Machine -- Wargus differs: tw=2 */
  { "Gnomish Flying Machine", "balloon", 0x00000082ul, W2UX_RECT_SELECT | W2UX_COWARD | W2UX_DETECT_INVIS,
    150, 500, 100, 0, 40, {38, 38, 38, 38},
    W2_RACE_HUMAN, W2_KIND_AIR, 1, 1, 63, 63, 80, 80,
    2, 0, 0, 0, 1, 9, 17, 65, -1,
    0, 2, 19, 15, 40, 0, 0, 0,
    W2M_NONE, 0, W2A_BALLOON, N, 28, W2U_INVENTOR, N, W2U_ZEPPELIN, 0, 0, 0, 0,
    99, 100, 101, 5, 22 },
  /* 0x29 Goblin Zeppelin -- Wargus differs: tw=2 */
  { "Goblin Zeppelin", "zeppelin", 0x00000082ul, W2UX_RECT_SELECT | W2UX_COWARD | W2UX_DETECT_INVIS,
    150, 500, 100, 0, 40, {63, 63, 63, 63},
    W2_RACE_ORC, W2_KIND_AIR, 1, 1, 63, 63, 72, 72,
    2, 0, 0, 0, 1, 9, 17, 65, -1,
    0, 2, 19, 15, 40, 0, 0, 0,
    W2M_NONE, 0, W2A_GOBLIN_ZEPPELIN, N, 29, W2U_ALCHEMIST, N, W2U_BALLOON, 0, 0, 0, 0,
    103, 104, 105, 12, 22 },
  /* 0x2A Gryphon Rider -- Wargus differs: tw=2 */
  { "Gryphon Rider", "gryphon-rider", 0x08080082ul, W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    100, 2500, 0, 0, 150, {35, 35, 35, 35},
    W2_RACE_HUMAN, W2_KIND_AIR, 1, 1, 63, 63, 80, 80,
    5, 0, 16, 0, 4, 6, 14, 250, -1,
    7, 1, 8, 6, 65, 0, 0, 0,
    W2M_GRIFFON_HAMMER, 0, W2A_GRYPHON_RIDER, N, 30, W2U_GRYPHON_AVIARY, N, W2U_DRAGON, 0, 0, 0, 0,
    106, 78, 107, 5, 6 },
  /* 0x2B Dragon -- Wargus differs: tw=2 */
  { "Dragon", "dragon", 0x08080082ul, W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    100, 2500, 0, 0, 150, {36, 36, 36, 36},
    W2_RACE_ORC, W2_KIND_AIR, 1, 1, 71, 71, 88, 80,
    5, 0, 16, 0, 4, 6, 14, 250, -1,
    7, 1, 8, 6, 65, 0, 0, 0,
    W2M_DRAGON_BREATH, 0, W2A_DRAGON, N, 31, W2U_DRAGON_ROOST, N, W2U_GRYPHON_RIDER, 0, 0, 0, 0,
    108, 95, 109, 12, 22 },
  /* 0x2C Turalyon */
  { "Turalyon", "knight-rider", 0x080A0001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_MANA | W2UX_RECT_SELECT,
    180, 800, 100, 0, 110, {51, 51, 51, 51},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    10, 14, 5, 0, 1, 6, 13, 90, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 1, W2A_KNIGHT_RIDER, N, 195, N, N, N, 0, 0, 0, 0,
    110, 26, N, 5, 6 },
  /* 0x2D Eye of Kilrogg -- Wargus differs: bd=1 */
  { "Eye of Kilrogg", "eye-of-vision", 0x00000082ul, W2UX_RECT_SELECT | W2UX_DETECT_INVIS,
    100, 0, 0, 0, 0, {37, 37, 37, 37},
    W2_RACE_ORC, W2_KIND_AIR, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 1, 3, 42, 0, 0,
    0, 2, 20, 10, 0, 0, 3, 0,
    W2M_NONE, 0, W2A_EYE_OF_VISION, N, 111, N, N, N, 0, 0, 0, 0,
    20, N, N, 12, N },
  /* 0x2E Danath */
  { "Danath", "arthor-literios", 0x08080001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT,
    220, 600, 0, 0, 50, {45, 45, 45, 45},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    8, 15, 8, 0, 1, 6, 10, 60, -1,
    1, 1, 6, 4, 60, 0, 0, 0,
    W2M_NONE, 1, W2A_ARTHOR_LITERIOS, N, 188, N, N, N, 0, 0, 0, 0,
    111, 3, N, 5, 6 },
  /* 0x2F Korgath Bladefist */
  { "Korgath Bladefist", "quick-blade", 0x08080001ul, W2UX_EXPANSION | W2UX_STATS_WARGUS | W2UX_RECT_SELECT,
    240, 600, 0, 0, 50, {46, 46, 46, 46},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 72, 72,
    8, 16, 6, 0, 1, 5, 10, 60, -1,
    1, 1, 6, 4, 60, 0, 0, 0,
    W2M_NONE, 2, W2A_QUICK_BLADE, N, 186, N, N, N, 0, 0, 0, 0,
    112, 10, N, 12, 13 },
  /* 0x30 (unused) */
  { "", "", 0x00000000ul, W2UX_UNUSED,
    0, 0, 0, 0, 0, {0, 0, 0, 0},
    W2_RACE_NEUTRAL, W2_KIND_NONE, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, N, N, N, N, N, N, 0, 0, 0, 0,
    N, N, N, N, N },
  /* 0x31 Cho'gall */
  { "Cho'gall", "double-head", 0x088A0001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    100, 1100, 50, 0, 120, {52, 52, 52, 52},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    0, 10, 5, 0, 1, 5, 13, 100, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 2, W2A_DOUBLE_HEAD, N, 36, N, N, N, 0, 0, 0, 0,
    60, 61, N, 12, 13 },
  /* 0x32 Lothar */
  { "Lothar", "wise-man", 0x08880001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 900, 100, 0, 120, {51, 51, 51, 51},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 5, 13, 100, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 1, W2A_WISE_MAN, N, 32, N, N, N, 0, 0, 0, 0,
    25, 26, N, 5, 6 },
  /* 0x33 Gul'dan */
  { "Gul'dan", "ice-bringer", 0x088A8001ul, W2UX_MANA | W2UX_RECT_SELECT,
    40, 1200, 0, 0, 120, {58, 58, 58, 58},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 33, 33, 72, 72,
    0, 0, 3, 0, 3, 8, 8, 120, -1,
    7, 1, 10, 8, 70, 0, 0, 0,
    W2M_TOUCH_OF_DEATH, 0, W2A_ICE_BRINGER, N, 33, N, N, N, 0, 0, 0, 0,
    50, 51, N, 12, 13 },
  /* 0x34 Uther Lightbringer */
  { "Uther Lightbringer", "man-of-light", 0x088A0001ul, W2UX_WEAPON_UPG | W2UX_ARMOR_UPG | W2UX_RECT_SELECT,
    90, 900, 100, 0, 120, {51, 51, 51, 51},
    W2_RACE_HUMAN, W2_KIND_LAND, 1, 1, 42, 42, 72, 72,
    4, 8, 4, 0, 1, 5, 13, 100, -1,
    1, 1, 7, 5, 65, 0, 0, 0,
    W2M_NONE, 1, W2A_MAN_OF_LIGHT, N, 34, N, N, N, 0, 0, 0, 0,
    55, 56, N, 5, 6 },
  /* 0x35 Zuljin -- Wargus differs: armor=5, bd=10, hp=120, pd=18, sight=9 */
  { "Zuljin", "sharp-axe", 0x08880001ul, W2UX_WEAPON_UPG | W2UX_RECT_SELECT,
    40, 500, 50, 0, 120, {54, 54, 54, 54},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 36, 36, 72, 72,
    0, 3, 6, 0, 5, 6, 10, 70, -1,
    7, 1, 8, 6, 55, 0, 0, 0,
    W2M_AXE, 2, W2A_SHARP_AXE, N, 35, N, N, N, 0, 0, 0, 0,
    38, 41, N, 12, 13 },
  /* 0x36 (unused) */
  { "", "", 0x00000000ul, W2UX_UNUSED,
    0, 0, 0, 0, 0, {0, 0, 0, 0},
    W2_RACE_NEUTRAL, W2_KIND_NONE, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, N, N, N, N, N, N, 0, 0, 0, 0,
    N, N, N, N, N },
  /* 0x37 Skeleton */
  { "Skeleton", "skeleton", 0x08088011ul, W2UX_RECT_SELECT,
    40, 0, 0, 0, 0, {69, 69, 69, 69},
    W2_RACE_ORC, W2_KIND_LAND, 1, 1, 31, 31, 56, 56,
    0, 6, 3, 0, 1, 3, 8, 0, -1,
    1, 1, 4, 2, 55, 0, 100, 0,
    W2M_NONE, 0, W2A_SKELETON, N, 114, N, N, N, 0, 0, 0, 0,
    20, 20, N, 12, 13 },
  /* 0x38 Daemon -- Wargus differs: armor=3, bd=10, gold=500, maxr=3, oil=50, pd=2, time=70 */
  { "Daemon", "daemon", 0x08080092ul, W2UX_DETECT_INVIS,
    60, 0, 0, 0, 100, {70, 70, 70, 70},
    W2_RACE_ORC, W2_KIND_AIR, 1, 1, 31, 31, 72, 72,
    2, 9, 1, 0, 2, 5, 14, 0, -1,
    7, 1, 7, 5, 63, 0, 0, 0,
    W2M_DAEMON_FIRE, 0, W2A_DAEMON, N, 37, N, N, N, 0, 0, 0, 0,
    20, N, N, 12, 22 },
  /* 0x39 Critter -- Wargus differs: bd=80 */
  { "Critter", "critter", 0x08000011ul, 0,
    5, 0, 0, 0, 1, {64, 66, 65, 65},
    W2_RACE_NEUTRAL, W2_KIND_LAND, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 1, 2, 3, 0, -1,
    0, 2, 20, 10, 37, 0, 0, 0,
    W2M_NONE, 0, W2A_CRITTER, N, 115, N, N, N, 0, 0, 0, 0,
    N, N, N, 12, 22 },
  /* 0x3A Farm -- Wargus differs: sight=1 */
  { "Farm", "farm", 0x00000020ul, W2UX_EXPLODES,
    400, 500, 250, 0, 100, {92, 134, 173, 92},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 0, 0, 0, 0, 3, 0, 100, 4,
    0, 0, 0, 0, 20, 45, 0, 6,
    W2M_NONE, 5, W2A_BUILDING, W2C_LAND, 38, W2U_PEASANT, N, W2U_PIG_FARM, 0, 0, 0, 0,
    113, N, N, 114, 115 },
  /* 0x3B Pig Farm -- Wargus differs: sight=2 */
  { "Pig Farm", "pig-farm", 0x00000020ul, W2UX_EXPLODES,
    400, 500, 250, 0, 100, {93, 135, 174, 93},
    W2_RACE_ORC, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 0, 0, 0, 0, 3, 0, 100, 4,
    0, 0, 0, 0, 20, 45, 0, 6,
    W2M_NONE, 5, W2A_BUILDING, W2C_LAND, 39, W2U_PEON, N, W2U_FARM, 0, 0, 0, 0,
    116, N, N, 117, 115 },
  /* 0x3C Human Barracks -- Wargus differs: sight=1 */
  { "Human Barracks", "human-barracks", 0x00000020ul, W2UX_EXPLODES,
    800, 700, 450, 0, 160, {94, 136, 94, 94},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 30, 35, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 42, W2U_PEASANT, N, W2U_ORC_BARRACKS, 0, 0, 0, 0,
    20, N, N, 114, 115 },
  /* 0x3D Orc Barracks -- Wargus differs: sight=1 */
  { "Orc Barracks", "orc-barracks", 0x00000020ul, W2UX_EXPLODES,
    800, 700, 450, 0, 160, {95, 137, 95, 95},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 30, 35, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 43, W2U_PEON, N, W2U_HUMAN_BARRACKS, 0, 0, 0, 0,
    20, N, N, 117, 115 },
  /* 0x3E Church -- Wargus differs: sight=1 */
  { "Church", "church", 0x00000020ul, W2UX_EXPLODES,
    700, 900, 500, 0, 240, {96, 138, 96, 96},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 175, 0,
    0, 0, 0, 0, 15, 35, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 62, W2U_PEASANT, N, W2U_ALTAR_OF_STORMS, 0, 0, 0, 0,
    118, N, N, 114, 115 },
  /* 0x3F Altar of Storms -- Wargus differs: sight=1 */
  { "Altar of Storms", "altar-of-storms", 0x00000020ul, W2UX_EXPLODES,
    700, 900, 500, 0, 240, {97, 139, 97, 97},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 175, 0,
    0, 0, 0, 0, 15, 35, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 63, W2U_PEON, N, W2U_CHURCH, 0, 0, 0, 0,
    119, N, N, 117, 115 },
  /* 0x40 Scout Tower */
  { "Scout Tower", "human-watch-tower", 0x000000A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    100, 550, 200, 0, 95, {98, 140, 98, 98},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 0, 0, 0, 0, 9, 0, 60, 0,
    7, 0, 0, 0, 55, 50, 0, 6,
    W2M_NONE, 5, W2A_BUILDING, W2C_LAND, 60, W2U_PEASANT, N, W2U_ORC_WATCH_TOWER, 0, 0, 0, 0,
    20, N, N, 114, 115 },
  /* 0x41 Watch Tower */
  { "Watch Tower", "orc-watch-tower", 0x000000A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    100, 550, 200, 0, 95, {99, 141, 99, 99},
    W2_RACE_ORC, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 0, 0, 0, 0, 9, 0, 60, 0,
    7, 0, 0, 0, 55, 50, 0, 6,
    W2M_NONE, 5, W2A_BUILDING, W2C_LAND, 61, W2U_PEON, N, W2U_HUMAN_WATCH_TOWER, 0, 0, 0, 0,
    20, N, N, 117, 115 },
  /* 0x42 Stables -- Wargus differs: sight=1 */
  { "Stables", "stables", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 300, 0, 210, {104, 146, 104, 104},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 15, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 56, W2U_PEASANT, N, W2U_OGRE_MOUND, 0, 0, 0, 0,
    120, N, N, 114, 115 },
  /* 0x43 Ogre Mound -- Wargus differs: sight=1 */
  { "Ogre Mound", "ogre-mound", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 300, 0, 210, {105, 147, 105, 105},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 15, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 57, W2U_PEON, N, W2U_STABLES, 0, 0, 0, 0,
    121, N, N, 117, 115 },
  /* 0x44 Gnomish Inventor -- Wargus differs: sight=1 */
  { "Gnomish Inventor", "inventor", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 400, 0, 230, {90, 132, 90, 90},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 58, W2U_PEASANT, N, W2U_ALCHEMIST, 0, 0, 0, 0,
    122, N, N, 114, 115 },
  /* 0x45 Goblin Alchemist -- Wargus differs: sight=1 */
  { "Goblin Alchemist", "alchemist", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 400, 0, 230, {91, 133, 91, 91},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 59, W2U_PEON, N, W2U_INVENTOR, 0, 0, 0, 0,
    123, N, N, 117, 115 },
  /* 0x46 Gryphon Aviary -- Wargus differs: sight=1 */
  { "Gryphon Aviary", "gryphon-aviary", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 400, 0, 280, {88, 130, 88, 88},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 72, W2U_PEASANT, N, W2U_DRAGON_ROOST, 0, 0, 0, 0,
    124, N, N, 114, 115 },
  /* 0x47 Dragon Roost -- Wargus differs: sight=1 */
  { "Dragon Roost", "dragon-roost", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 400, 0, 280, {89, 131, 89, 89},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 73, W2U_PEON, N, W2U_GRYPHON_AVIARY, 0, 0, 0, 0,
    125, N, N, 117, 115 },
  /* 0x48 Human Shipyard -- Wargus differs: sight=1 */
  { "Human Shipyard", "human-shipyard", 0x01010020ul, W2UX_EXPLODES | W2UX_SHORE,
    1100, 800, 450, 0, 170, {108, 150, 108, 108},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 30, 20, 0, 7,
    W2M_NONE, 7, W2A_BUILDING, W2C_HUMAN_SHIPYARD, 48, W2U_PEASANT, N, W2U_ORC_SHIPYARD, 4, 0, 0, 0,
    126, N, N, 114, 115 },
  /* 0x49 Orc Shipyard -- Wargus differs: sight=1 */
  { "Orc Shipyard", "orc-shipyard", 0x01010020ul, W2UX_EXPLODES | W2UX_SHORE,
    1100, 800, 450, 0, 170, {109, 151, 109, 109},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 30, 20, 0, 8,
    W2M_NONE, 7, W2A_BUILDING, W2C_ORC_SHIPYARD, 49, W2U_PEON, N, W2U_HUMAN_SHIPYARD, 4, 0, 0, 0,
    126, N, N, 117, 115 },
  /* 0x4A Town Hall -- Wargus differs: sight=1 */
  { "Town Hall", "town-hall", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1200, 1200, 800, 0, 200, {100, 142, 100, 100},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 4, 4, 126, 126, 128, 128,
    20, 0, 0, 0, 0, 4, 0, 255, 1,
    0, 0, 0, 0, 35, 45, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 40, W2U_PEASANT, N, W2U_GREAT_HALL, 3, 0, 0, 0,
    20, N, N, 114, 115 },
  /* 0x4B Great Hall -- Wargus differs: sight=1 */
  { "Great Hall", "great-hall", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1200, 1200, 800, 0, 200, {101, 143, 101, 101},
    W2_RACE_ORC, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    20, 0, 0, 0, 0, 4, 0, 255, 1,
    0, 0, 0, 0, 35, 45, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 41, W2U_PEON, N, W2U_TOWN_HALL, 3, 0, 0, 0,
    20, N, N, 117, 115 },
  /* 0x4C Elven Lumber Mill -- Wargus differs: sight=1 */
  { "Elven Lumber Mill", "elven-lumber-mill", 0x00040020ul, W2UX_EXPLODES,
    600, 600, 450, 0, 150, {102, 144, 175, 102},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 25, 15, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 44, W2U_PEASANT, N, W2U_TROLL_LUMBER_MILL, 2, 2, 25, 0,
    127, N, N, 114, 115 },
  /* 0x4D Troll Lumber Mill -- Wargus differs: sight=1 */
  { "Troll Lumber Mill", "troll-lumber-mill", 0x00040020ul, W2UX_EXPLODES,
    600, 600, 450, 0, 150, {103, 145, 176, 103},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 25, 15, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 45, W2U_PEON, N, W2U_ELVEN_LUMBER_MILL, 2, 2, 25, 0,
    127, N, N, 117, 115 },
  /* 0x4E Human Foundry -- Wargus differs: sight=1 */
  { "Human Foundry", "human-foundry", 0x00010020ul, W2UX_EXPLODES | W2UX_SHORE,
    750, 700, 400, 400, 200, {110, 152, 110, 110},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 175, 0,
    0, 0, 0, 0, 15, 20, 0, 13,
    W2M_NONE, 7, W2A_BUILDING, W2C_HUMAN_FOUNDRY, 52, W2U_PEASANT, N, W2U_ORC_FOUNDRY, 0, 0, 0, 0,
    128, N, N, 114, 115 },
  /* 0x4F Orc Foundry -- Wargus differs: sight=1 */
  { "Orc Foundry", "orc-foundry", 0x00010020ul, W2UX_EXPLODES | W2UX_SHORE,
    750, 700, 400, 400, 200, {111, 153, 111, 111},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 175, 0,
    0, 0, 0, 0, 15, 20, 0, 14,
    W2M_NONE, 7, W2A_BUILDING, W2C_ORC_FOUNDRY, 53, W2U_PEON, N, W2U_HUMAN_FOUNDRY, 0, 0, 0, 0,
    128, N, N, 117, 115 },
  /* 0x50 Mage Tower -- Wargus differs: sight=1 */
  { "Mage Tower", "mage-tower", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 200, 0, 240, {84, 160, 84, 84},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 125, 0,
    0, 0, 0, 0, 35, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 64, W2U_PEASANT, N, W2U_TEMPLE_OF_THE_DAMNED, 0, 0, 0, 0,
    129, N, N, 114, 115 },
  /* 0x51 Temple of the Damned -- Wargus differs: sight=1 */
  { "Temple of the Damned", "temple-of-the-damned", 0x00000020ul, W2UX_EXPLODES,
    500, 1000, 200, 0, 240, {85, 161, 85, 85},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 125, 0,
    0, 0, 0, 0, 35, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 65, W2U_PEON, N, W2U_MAGE_TOWER, 0, 0, 0, 0,
    130, N, N, 117, 115 },
  /* 0x52 Human Blacksmith -- Wargus differs: sight=1 */
  { "Human Blacksmith", "human-blacksmith", 0x00000020ul, W2UX_EXPLODES,
    775, 800, 450, 100, 170, {106, 148, 106, 106},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 46, W2U_PEASANT, N, W2U_ORC_BLACKSMITH, 0, 0, 0, 0,
    131, N, N, 114, 115 },
  /* 0x53 Orc Blacksmith -- Wargus differs: sight=1 */
  { "Orc Blacksmith", "orc-blacksmith", 0x00000020ul, W2UX_EXPLODES,
    775, 800, 450, 100, 170, {107, 149, 107, 107},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 15, 20, 0, 6,
    W2M_NONE, 6, W2A_BUILDING, W2C_LAND, 47, W2U_PEON, N, W2U_HUMAN_BLACKSMITH, 0, 0, 0, 0,
    131, N, N, 117, 115 },
  /* 0x54 Human Refinery -- Wargus differs: sight=1 */
  { "Human Refinery", "human-refinery", 0x01010020ul, W2UX_EXPLODES | W2UX_SHORE,
    600, 800, 350, 200, 200, {112, 154, 112, 112},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 225, 0,
    0, 0, 0, 0, 25, 20, 0, 11,
    W2M_NONE, 7, W2A_BUILDING, W2C_HUMAN_REFINERY, 50, W2U_PEASANT, N, W2U_ORC_REFINERY, 4, 3, 25, 0,
    132, N, N, 114, 115 },
  /* 0x55 Orc Refinery -- Wargus differs: sight=1 */
  { "Orc Refinery", "orc-refinery", 0x01010020ul, W2UX_EXPLODES | W2UX_SHORE,
    600, 800, 350, 200, 200, {113, 155, 113, 113},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 225, 0,
    0, 0, 0, 0, 25, 20, 0, 12,
    W2M_NONE, 7, W2A_BUILDING, W2C_ORC_REFINERY, 51, W2U_PEON, N, W2U_HUMAN_REFINERY, 4, 3, 25, 0,
    132, N, N, 117, 115 },
  /* 0x56 Human Oil Platform -- Wargus differs: sight=1 */
  { "Human Oil Platform", "human-oil-platform", 0x00000820ul, W2UX_EXPLODES | W2UX_RESOURCE,
    650, 700, 450, 0, 160, {114, 156, 177, 114},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 20, 20, 0, 9,
    W2M_NONE, 7, W2A_OIL_PLATFORM, W2C_HUMAN_OIL_WELL, 54, W2U_HUMAN_OIL_TANKER, N, W2U_ORC_OIL_PLATFORM, 0, 0, 0, 0,
    133, N, N, 114, 115 },
  /* 0x57 Orc Oil Platform -- Wargus differs: sight=1 */
  { "Orc Oil Platform", "orc-oil-platform", 0x00000820ul, W2UX_EXPLODES | W2UX_RESOURCE,
    650, 700, 450, 0, 160, {115, 157, 178, 115},
    W2_RACE_ORC, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 200, 0,
    0, 0, 0, 0, 20, 20, 0, 10,
    W2M_NONE, 7, W2A_OIL_PLATFORM, W2C_ORC_OIL_WELL, 55, W2U_ORC_OIL_TANKER, N, W2U_HUMAN_OIL_PLATFORM, 0, 0, 0, 0,
    133, N, N, 117, 115 },
  /* 0x58 Keep -- Wargus differs: sight=3 */
  { "Keep", "keep", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1400, 2000, 1000, 200, 600, {86, 128, 86, 86},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    20, 0, 0, 0, 0, 6, 0, 200, 1,
    0, 0, 0, 0, 37, 40, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 66, N, W2U_TOWN_HALL, W2U_STRONGHOLD, 3, 1, 10, 0,
    20, N, N, 114, 115 },
  /* 0x59 Stronghold -- Wargus differs: sight=2 */
  { "Stronghold", "stronghold", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1400, 2000, 1000, 200, 600, {87, 129, 87, 87},
    W2_RACE_ORC, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    20, 0, 0, 0, 0, 6, 0, 200, 1,
    0, 0, 0, 0, 37, 40, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 67, N, W2U_GREAT_HALL, W2U_KEEP, 3, 1, 10, 0,
    20, N, N, 117, 115 },
  /* 0x5A Castle -- Wargus differs: sight=6 */
  { "Castle", "castle", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1600, 2500, 1200, 500, 1500, {116, 158, 116, 116},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    20, 0, 0, 0, 0, 9, 0, 200, 1,
    0, 0, 0, 0, 40, 50, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 70, N, W2U_KEEP, W2U_FORTRESS, 3, 1, 20, 0,
    20, N, N, 114, 115 },
  /* 0x5B Fortress -- Wargus differs: sight=6 */
  { "Fortress", "fortress", 0x00001020ul, W2UX_EXPLODES | W2UX_MAIN_HALL,
    1600, 2500, 1200, 500, 1500, {117, 159, 117, 117},
    W2_RACE_ORC, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    20, 0, 0, 0, 0, 9, 0, 200, 1,
    0, 0, 0, 0, 40, 50, 0, 6,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND, 71, N, W2U_STRONGHOLD, W2U_CASTLE, 3, 1, 20, 0,
    20, N, N, 117, 115 },
  /* 0x5C Gold Mine -- Wargus differs: sight=1 */
  { "Gold Mine", "gold-mine", 0x00400020ul, W2UX_EXPLODES | W2UX_RESOURCE,
    25500, 0, 0, 0, 0, {119, 162, 179, 179},
    W2_RACE_NEUTRAL, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    20, 0, 0, 0, 0, 3, 0, 150, 0,
    0, 0, 0, 0, 0, 0, 0, 6,
    W2M_NONE, 6, W2A_GOLD_MINE, W2C_LAND2, 74, N, N, N, 0, 0, 0, 0,
    134, N, N, 12, 115 },
  /* 0x5D Oil Patch */
  { "Oil Patch", "oil-patch", 0x00200020ul, W2UX_RESOURCE | W2UX_INDESTRUCTIBLE,
    0, 0, 0, 0, 0, {118, 118, 180, 180},
    W2_RACE_NEUTRAL, W2_KIND_BUILDING, 3, 3, 95, 95, 96, 96,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, W2A_BUILDING, N, 79, N, N, N, 0, 0, 0, 0,
    20, N, N, N, 115 },
  /* 0x5E Human Start Location */
  { "Human Start Location", "human-start-location", 0x00000000ul, W2UX_INDESTRUCTIBLE,
    0, 0, 0, 0, 0, {164, 164, 164, 164},
    W2_RACE_HUMAN, W2_KIND_MARKER, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, W2A_BUILDING, N, 91, N, N, W2U_ORC_START_LOCATION, 0, 0, 0, 0,
    N, N, N, 114, 115 },
  /* 0x5F Orc Start Location */
  { "Orc Start Location", "orc-start-location", 0x00000000ul, W2UX_INDESTRUCTIBLE,
    0, 0, 0, 0, 0, {165, 165, 165, 165},
    W2_RACE_ORC, W2_KIND_MARKER, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 0, W2A_BUILDING, N, 91, N, N, W2U_HUMAN_START_LOCATION, 0, 0, 0, 0,
    N, N, N, 117, 115 },
  /* 0x60 Human Guard Tower -- Wargus differs: prio=50 */
  { "Human Guard Tower", "human-guard-tower", 0x001800A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    130, 500, 150, 0, 200, {80, 169, 80, 80},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 4, 12, 0, 6, 9, 0, 140, 0,
    7, 0, 6, 6, 40, 50, 0, 6,
    W2M_ARROW, 5, W2A_HUMAN_GUARD_TOWER, W2C_LAND, 75, N, W2U_HUMAN_WATCH_TOWER, W2U_ORC_GUARD_TOWER, 0, 0, 0, 0,
    20, N, N, 114, 115 },
  /* 0x61 Orc Guard Tower -- Wargus differs: prio=50 */
  { "Orc Guard Tower", "orc-guard-tower", 0x001800A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    130, 500, 150, 0, 200, {81, 170, 81, 81},
    W2_RACE_ORC, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 4, 12, 0, 6, 9, 0, 140, 0,
    7, 0, 6, 6, 40, 50, 0, 6,
    W2M_ARROW, 5, W2A_ORC_GUARD_TOWER, W2C_LAND, 77, N, W2U_ORC_WATCH_TOWER, W2U_HUMAN_GUARD_TOWER, 0, 0, 0, 0,
    20, N, N, 117, 115 },
  /* 0x62 Human Cannon Tower -- Wargus differs: prio=60 */
  { "Human Cannon Tower", "human-cannon-tower", 0x001800A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    160, 1000, 300, 0, 250, {82, 171, 82, 82},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 50, 0, 2, 7, 9, 0, 190, 0,
    3, 0, 7, 7, 40, 50, 0, 6,
    W2M_SMALL_CANNON, 5, W2A_HUMAN_CANNON_TOWER, W2C_LAND, 76, N, W2U_HUMAN_WATCH_TOWER, W2U_ORC_CANNON_TOWER, 0, 0, 0, 0,
    20, N, N, 114, 115 },
  /* 0x63 Orc Cannon Tower -- Wargus differs: prio=60 */
  { "Orc Cannon Tower", "orc-cannon-tower", 0x001800A0ul, W2UX_EXPLODES | W2UX_DETECT_INVIS,
    160, 1000, 300, 0, 250, {83, 172, 83, 83},
    W2_RACE_ORC, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 50, 0, 2, 7, 9, 0, 190, 0,
    3, 0, 7, 7, 40, 50, 0, 6,
    W2M_SMALL_CANNON, 5, W2A_ORC_CANNON_TOWER, W2C_LAND, 78, N, W2U_ORC_WATCH_TOWER, W2U_HUMAN_CANNON_TOWER, 0, 0, 0, 0,
    N, N, N, 117, 115 },
  /* 0x64 Circle of Power */
  { "Circle of Power", "circle-of-power", 0x00000020ul, W2UX_EXPLODES | W2UX_INDESTRUCTIBLE,
    0, 0, 0, 0, 0, {166, 166, 166, 166},
    W2_RACE_NEUTRAL, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 5, W2A_BUILDING, N, 81, N, N, N, 0, 0, 0, 0,
    N, N, N, 114, 115 },
  /* 0x65 Dark Portal -- Wargus differs: gold=3000, oil=1000, sight=4, time=100, wood=3000 */
  { "Dark Portal", "dark-portal", 0x00000020ul, W2UX_EXPLODES | W2UX_BUILDER_OUTSIDE | W2UX_TELEPORTER,
    5000, 0, 0, 0, 0, {167, 184, 185, 185},
    W2_RACE_NEUTRAL, W2_KIND_BUILDING, 4, 4, 127, 127, 128, 128,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    W2M_NONE, 8, W2A_BUILDING, W2C_LAND2, 80, N, N, N, 0, 0, 0, 0,
    N, N, N, 117, 115 },
  /* 0x66 Runestone */
  { "Runestone", "runestone", 0x00000020ul, W2UX_EXPLODES | W2UX_BUILDER_OUTSIDE,
    5000, 900, 500, 0, 150, {181, 186, 181, 181},
    W2_RACE_NEUTRAL, W2_KIND_BUILDING, 2, 2, 63, 63, 64, 64,
    20, 0, 0, 0, 0, 4, 0, 175, 0,
    0, 0, 0, 0, 15, 35, 0, 6,
    W2M_NONE, 5, W2A_BUILDING, W2C_LAND2, 82, N, N, N, 0, 0, 0, 0,
    N, N, N, 117, 115 },
  /* 0x67 Wall -- Wargus differs: armor=20 */
  { "Wall", "human-wall", 0x00000020ul, W2UX_EXPLODES,
    40, 20, 10, 0, 1, {168, 273, 274, 168},
    W2_RACE_HUMAN, W2_KIND_BUILDING, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 0, 1, 0, 30, 0,
    0, 0, 0, 0, 0, 45, 0, 15,
    W2M_NONE, 4, W2A_BUILDING, W2C_WALL, 92, W2U_PEASANT, N, W2U_ORC_WALL, 0, 0, 0, 0,
    N, N, N, 114, 115 },
  /* 0x68 Wall -- Wargus differs: armor=20 */
  { "Wall", "orc-wall", 0x00000020ul, W2UX_EXPLODES,
    40, 20, 10, 0, 1, {168, 273, 274, 168},
    W2_RACE_ORC, W2_KIND_BUILDING, 1, 1, 31, 31, 32, 32,
    0, 0, 0, 0, 0, 1, 0, 30, 0,
    0, 0, 0, 0, 0, 45, 0, 15,
    W2M_NONE, 4, W2A_BUILDING, W2C_WALL, 93, W2U_PEON, N, W2U_HUMAN_WALL, 0, 0, 0, 0,
    N, N, N, 117, 115 },
};

const w2_anim_step w2_anim_steps[] = {
  /* 0: human-dead-body Death */
  {0,200,0,0x04}, {10,200,0,0x00}, {15,200,0,0x00}, {20,200,0,0x00}, {25,200,0,0x00}, {25,1,0,0x28},
  /* 6: orc-dead-body Death */
  {5,200,0,0x04}, {10,200,0,0x00}, {15,200,0,0x00}, {20,200,0,0x00}, {25,200,0,0x00}, {25,1,0,0x28},
  /* 12: dead-sea-body Death */
  {30,100,0,0x04}, {30,100,0,0x00}, {30,1,0,0x28},
  /* 15: destroyed-place Death */
  {0,200,0,0x04}, {1,200,0,0x00}, {1,1,0,0x28},
  /* 18: destroyed-place-water Death */
  {2,200,0,0x04}, {3,200,0,0x00}, {1,1,0,0x28},
  /* 21: footman Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 23: footman Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 35: footman Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 41: footman Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 45: grunt Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 47: grunt Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 59: grunt Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 65: grunt Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 69: peasant Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 71: peasant Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 83: peasant Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,136,0x03}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 90: peasant Death */
  {50,3,0,0x04}, {55,3,0,0x00}, {60,100,0,0x00}, {60,1,0,0x28},
  /* 94: peasant Harvest_wood */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,137,0x02}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 101: peasant Repair */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,136,0x02}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 108: peon Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 110: peon Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 122: peon Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,136,0x03}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 129: peon Death */
  {50,3,0,0x04}, {55,3,0,0x00}, {60,100,0,0x00}, {60,1,0,0x28},
  /* 133: peon Harvest_wood */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,137,0x02}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 140: peon Repair */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,136,0x02}, {45,3,0,0x00}, {25,7,0,0x00}, {25,1,0,0x08},
  /* 147: ballista Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 149: ballista Move */
  {0,1,0,0x04}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00},
  {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00},
  {0,1,2,0x08},
  /* 166: ballista Attack */
  {10,25,0,0x04}, {15,25,138,0x03}, {15,100,0,0x00}, {0,49,0,0x00}, {0,1,0,0x08},
  /* 171: catapult Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 173: catapult Move */
  {0,1,0,0x04}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00},
  {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00}, {0,2,2,0x00}, {5,2,2,0x00},
  {0,1,2,0x08},
  /* 190: catapult Attack */
  {15,4,138,0x07}, {10,4,0,0x00}, {15,3,0,0x00}, {10,2,0,0x00}, {15,2,0,0x00}, {10,30,0,0x00}, {15,4,0,0x00}, {15,100,0,0x00},
  {0,50,0,0x00}, {0,1,0,0x08},
  /* 200: knight Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 202: knight Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,2,4,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,3,0x00}, {15,2,4,0x00}, {20,1,3,0x00},
  {20,1,3,0x00}, {0,1,3,0x08},
  /* 212: knight Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 218: knight Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 224: ogre Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 226: ogre Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,1,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,1,3,0x00}, {15,1,3,0x00},
  {15,1,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 238: ogre Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,139,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 244: ogre Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 250: archer Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 252: archer Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 264: archer Attack */
  {25,10,0,0x04}, {30,10,140,0x03}, {0,44,0,0x00}, {0,1,0,0x08},
  /* 268: archer Death */
  {35,3,0,0x04}, {40,3,0,0x00}, {45,100,0,0x00}, {45,1,0,0x28},
  /* 272: axethrower Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 274: axethrower Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 286: axethrower Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,12,141,0x03}, {0,52,0,0x00}, {0,1,0,0x08},
  /* 292: axethrower Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 296: mage Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 298: mage Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,2,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,2,3,0x00}, {0,1,2,0x08},
  /* 310: mage Attack */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,142,0x03}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 316: mage Death */
  {45,5,0,0x04}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {65,5,0,0x00}, {70,5,0,0x00}, {75,5,0,0x00}, {75,1,0,0x28},
  /* 324: mage SpellCast */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,0,0x01}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 330: death-knight Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 332: death-knight Move */
  {0,2,3,0x04}, {5,2,3,0x00}, {5,2,4,0x00}, {10,2,3,0x00}, {10,2,3,0x00}, {15,2,3,0x00}, {15,2,4,0x00}, {20,2,3,0x00},
  {20,2,3,0x00}, {0,1,3,0x08},
  /* 342: death-knight Attack */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,143,0x03}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 348: death-knight Death */
  {45,5,0,0x04}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {60,1,0,0x28},
  /* 353: death-knight SpellCast */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,0,0x01}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 359: paladin Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 361: paladin Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,2,4,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,3,0x00}, {15,2,4,0x00}, {20,1,3,0x00},
  {20,1,3,0x00}, {0,1,3,0x08},
  /* 371: paladin Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 377: paladin Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 383: paladin SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 389: ogre-mage Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 391: ogre-mage Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,1,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,1,3,0x00}, {15,1,3,0x00},
  {15,1,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 403: ogre-mage Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,139,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 409: ogre-mage Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 415: ogre-mage SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 421: dwarves Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 423: dwarves Move */
  {0,2,3,0x04}, {10,1,3,0x00}, {10,2,4,0x00}, {25,1,3,0x00}, {25,2,3,0x00}, {40,1,3,0x00}, {40,2,4,0x00}, {55,1,3,0x00},
  {55,1,3,0x00}, {0,1,3,0x08},
  /* 433: dwarves Attack */
  {15,3,0,0x04}, {30,5,135,0x03}, {45,3,0,0x00}, {0,13,0,0x00}, {0,1,0,0x08},
  /* 438: dwarves Death */
  {5,3,0,0x04}, {20,3,0,0x00}, {35,3,0,0x00}, {50,3,0,0x00}, {60,3,0,0x00}, {60,1,0,0x28},
  /* 444: dwarves SpellCast */
  {15,3,0,0x04}, {30,5,0,0x01}, {45,3,0,0x00}, {0,13,0,0x00}, {0,1,0,0x08},
  /* 449: goblin-sappers Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 451: goblin-sappers Move */
  {0,1,3,0x04}, {10,1,3,0x00}, {10,1,2,0x00}, {25,2,3,0x00}, {25,1,3,0x00}, {40,1,2,0x00}, {40,1,3,0x00}, {55,1,3,0x00},
  {55,1,2,0x00}, {65,2,3,0x00}, {65,1,3,0x00}, {0,1,2,0x08},
  /* 463: goblin-sappers Attack */
  {15,3,0,0x04}, {30,5,135,0x03}, {45,3,0,0x00}, {0,13,0,0x00}, {0,1,0,0x08},
  /* 468: goblin-sappers Death */
  {5,3,0,0x04}, {20,3,0,0x00}, {35,3,0,0x00}, {50,3,0,0x00}, {60,3,0,0x00}, {70,3,0,0x00}, {70,1,0,0x28},
  /* 475: goblin-sappers SpellCast */
  {15,3,0,0x04}, {30,5,0,0x01}, {45,3,0,0x00}, {0,13,0,0x00}, {0,1,0,0x08},
  /* 480: ranger Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 482: ranger Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 494: ranger Attack */
  {25,10,0,0x04}, {30,10,140,0x03}, {0,44,0,0x00}, {0,1,0,0x08},
  /* 498: ranger Death */
  {35,3,0,0x04}, {40,3,0,0x00}, {45,100,0,0x00}, {45,1,0,0x28},
  /* 502: female-hero Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 504: female-hero Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 516: female-hero Attack */
  {25,10,0,0x04}, {30,10,140,0x03}, {0,44,0,0x00}, {0,1,0,0x08},
  /* 520: female-hero Death */
  {35,3,0,0x04}, {40,3,0,0x00}, {45,100,0,0x00}, {45,1,0,0x28},
  /* 524: flying-angel Still */
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  /* 528: flying-angel Move */
  {0,1,0,0x04}, {0,1,3,0x00}, {0,1,3,0x00}, {5,1,3,0x00}, {5,1,2,0x00}, {5,1,3,0x00}, {10,1,3,0x00}, {10,1,3,0x00},
  {10,1,3,0x00}, {15,1,3,0x00}, {15,1,3,0x00}, {0,1,3,0x08},
  /* 540: flying-angel Attack */
  {0,6,0,0x04}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {15,1,0,0x00}, {20,6,0,0x00}, {25,6,0,0x00}, {30,8,142,0x03},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,1,0,0x08},
  /* 573: flying-angel Death */
  {35,5,0,0x04}, {40,5,0,0x00}, {45,5,0,0x00}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {60,1,0,0x28},
  /* 580: white-mage Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 582: white-mage Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,2,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,2,3,0x00}, {0,1,2,0x08},
  /* 594: white-mage Attack */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,142,0x03}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 600: white-mage Death */
  {45,5,0,0x04}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {65,5,0,0x00}, {70,5,0,0x00}, {75,5,0,0x00}, {75,1,0,0x28},
  /* 608: white-mage SpellCast */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,0,0x01}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 614: human-oil-tanker Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 616: human-oil-tanker Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 632: human-oil-tanker Attack */
  {0,30,0,0x05}, {0,99,0,0x00}, {0,1,0,0x08},
  /* 635: human-oil-tanker Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 638: orc-oil-tanker Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 640: orc-oil-tanker Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 656: orc-oil-tanker Attack */
  {0,30,0,0x05}, {0,99,0,0x00}, {0,1,0,0x08},
  /* 659: orc-oil-tanker Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 662: human-transport Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 664: human-transport Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 680: human-transport Attack */
  {0,119,0,0x05}, {0,1,0,0x08},
  /* 682: human-transport Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 685: orc-transport Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 687: orc-transport Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 703: orc-transport Attack */
  {0,119,0,0x05}, {0,1,0,0x08},
  /* 705: orc-transport Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 708: elven-destroyer Still */
  {0,4,0,0x00}, {0,64,0,0x00}, {0,64,0,0x00}, {0,1,0,0x00},
  /* 712: elven-destroyer Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 728: elven-destroyer Attack */
  {0,119,144,0x07}, {0,1,0,0x08},
  /* 730: elven-destroyer Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 733: troll-destroyer Still */
  {0,4,0,0x00}, {0,64,0,0x00}, {0,64,0,0x00}, {0,1,0,0x00},
  /* 737: troll-destroyer Move */
  {0,1,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x08},
  /* 753: troll-destroyer Attack */
  {0,119,144,0x07}, {0,1,0,0x08},
  /* 755: troll-destroyer Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 758: battleship Still */
  {0,4,0,0x00}, {0,64,0,0x00}, {0,64,0,0x00}, {0,1,0,0x00},
  /* 762: battleship Move */
  {0,2,2,0x04}, {0,2,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,2,2,0x00},
  {0,1,2,0x00}, {0,2,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x08},
  /* 778: battleship Attack */
  {0,127,144,0x07}, {0,102,0,0x00}, {0,1,0,0x08},
  /* 781: battleship Death */
  {5,50,0,0x04}, {10,50,0,0x00}, {10,1,0,0x28},
  /* 784: fire-breeze Still */
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  /* 788: fire-breeze Move */
  {0,1,0,0x04}, {0,2,6,0x00}, {5,2,6,0x00}, {5,1,3,0x00}, {10,2,5,0x00}, {10,1,3,0x00}, {15,2,6,0x00}, {0,1,3,0x08},
  /* 796: fire-breeze Attack */
  {0,6,0,0x04}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {15,1,0,0x00}, {20,20,144,0x03}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,1,0,0x08},
  /* 827: fire-breeze Death */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,5,0,0x00}, {40,5,0,0x00}, {45,5,0,0x00}, {45,1,0,0x28},
  /* 833: gnomish-submarine Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 835: gnomish-submarine Move */
  {0,2,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x08},
  /* 851: gnomish-submarine Attack */
  {5,10,0,0x04}, {10,25,0,0x00}, {10,25,144,0x03}, {5,25,0,0x00}, {0,29,0,0x00}, {0,1,0,0x08},
  /* 857: gnomish-submarine Death */
  {0,1,0,0x2C},
  /* 858: orc-submarine Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 860: orc-submarine Move */
  {0,2,2,0x04}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00},
  {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,1,2,0x00}, {0,2,2,0x00}, {0,1,2,0x08},
  /* 876: orc-submarine Attack */
  {5,10,0,0x04}, {10,25,0,0x00}, {10,25,144,0x03}, {5,25,0,0x00}, {0,29,0,0x00}, {0,1,0,0x08},
  /* 882: orc-submarine Death */
  {0,1,0,0x2C},
  /* 883: balloon Still */
  {0,1,0,0x00}, {5,1,0,0x00}, {5,1,0,0x00}, {0,1,0,0x00},
  /* 887: balloon Move */
  {5,1,4,0x04}, {5,1,3,0x00}, {5,1,3,0x00}, {0,1,3,0x00}, {0,1,3,0x00}, {5,1,4,0x00}, {5,1,3,0x00}, {5,1,3,0x00},
  {0,1,3,0x00}, {0,1,3,0x08},
  /* 897: balloon Attack */
  {0,1,0,0x0C},
  /* 898: balloon Death */
  {0,1,0,0x2C},
  /* 899: goblin-zeppelin Still */
  {0,4,0,0x00}, {0,16,0,0x00}, {0,16,0,0x00}, {0,1,0,0x00},
  /* 903: goblin-zeppelin Move */
  {0,1,4,0x04}, {0,1,3,0x00}, {0,1,3,0x00}, {0,1,3,0x00}, {0,1,3,0x00}, {0,1,4,0x00}, {0,1,3,0x00}, {0,1,3,0x00},
  {0,1,3,0x00}, {0,1,3,0x08},
  /* 913: goblin-zeppelin Attack */
  {0,1,0,0x0C},
  /* 914: goblin-zeppelin Death */
  {0,1,0,0x2C},
  /* 915: gryphon-rider Still */
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  /* 919: gryphon-rider Move */
  {0,1,0,0x04}, {0,1,3,0x00}, {0,1,3,0x00}, {5,1,3,0x00}, {5,1,2,0x00}, {5,1,3,0x00}, {10,1,3,0x00}, {10,1,3,0x00},
  {10,1,3,0x00}, {15,1,3,0x00}, {15,1,3,0x00}, {0,1,3,0x08},
  /* 931: gryphon-rider Attack */
  {0,6,0,0x04}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {15,1,0,0x00}, {20,6,0,0x00}, {25,6,0,0x00}, {30,8,142,0x03},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  {0,1,0,0x08},
  /* 964: gryphon-rider Death */
  {35,5,0,0x04}, {40,5,0,0x00}, {45,5,0,0x00}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {60,1,0,0x28},
  /* 971: dragon Still */
  {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00},
  /* 975: dragon Move */
  {0,1,0,0x04}, {0,2,6,0x00}, {5,2,6,0x00}, {5,1,3,0x00}, {10,2,5,0x00}, {10,1,3,0x00}, {15,2,6,0x00}, {0,1,3,0x08},
  /* 983: dragon Attack */
  {0,6,0,0x04}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {15,1,0,0x00}, {20,20,144,0x03}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00},
  {10,6,0,0x00}, {15,6,0,0x00}, {0,6,0,0x00}, {5,6,0,0x00}, {10,6,0,0x00}, {15,6,0,0x00}, {0,1,0,0x08},
  /* 1014: dragon Death */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,5,0,0x00}, {40,5,0,0x00}, {45,5,0,0x00}, {45,1,0,0x28},
  /* 1020: knight-rider Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1022: knight-rider Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,2,4,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,3,0x00}, {15,2,4,0x00}, {20,1,3,0x00},
  {20,1,3,0x00}, {0,1,3,0x08},
  /* 1032: knight-rider Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1038: knight-rider Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 1044: knight-rider SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1050: eye-of-vision Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1052: eye-of-vision Move */
  {0,1,8,0x04}, {0,1,8,0x00}, {0,1,8,0x00}, {0,1,8,0x08},
  /* 1056: eye-of-vision Attack */
  {0,1,0,0x0C},
  /* 1057: arthor-literios Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1059: arthor-literios Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 1071: arthor-literios Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1077: arthor-literios Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 1081: quick-blade Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1083: quick-blade Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 1095: quick-blade Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1101: quick-blade Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 1105: double-head Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1107: double-head Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,1,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,1,3,0x00}, {15,1,3,0x00},
  {15,1,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 1119: double-head Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,139,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1125: double-head Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 1131: double-head SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1137: wise-man Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1139: wise-man Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,2,4,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,3,0x00}, {15,2,4,0x00}, {20,1,3,0x00},
  {20,1,3,0x00}, {0,1,3,0x08},
  /* 1149: wise-man Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1155: wise-man Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 1161: wise-man SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1167: ice-bringer Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1169: ice-bringer Move */
  {0,2,3,0x04}, {5,2,3,0x00}, {5,2,4,0x00}, {10,2,3,0x00}, {10,2,3,0x00}, {15,2,3,0x00}, {15,2,4,0x00}, {20,2,3,0x00},
  {20,2,3,0x00}, {0,1,3,0x08},
  /* 1179: ice-bringer Attack */
  {25,5,0,0x04}, {30,5,0,0x00}, {35,7,143,0x03}, {40,5,0,0x00}, {0,17,0,0x00}, {0,1,0,0x08},
  /* 1185: ice-bringer Death */
  {45,5,0,0x04}, {50,5,0,0x00}, {55,5,0,0x00}, {60,5,0,0x00}, {60,1,0,0x28},
  /* 1190: man-of-light Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1192: man-of-light Move */
  {0,1,3,0x04}, {5,1,3,0x00}, {5,2,4,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,3,0x00}, {15,2,4,0x00}, {20,1,3,0x00},
  {20,1,3,0x00}, {0,1,3,0x08},
  /* 1202: man-of-light Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,135,0x03}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1208: man-of-light Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {60,200,0,0x00}, {65,200,0,0x00}, {65,1,0,0x28},
  /* 1214: man-of-light SpellCast */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,5,0,0x01}, {0,10,0,0x00}, {0,1,0,0x08},
  /* 1220: sharp-axe Still */
  {0,4,0,0x00}, {0,1,1,0x10},
  /* 1222: sharp-axe Move */
  {0,2,3,0x04}, {5,1,3,0x00}, {5,2,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {15,1,3,0x00},
  {15,2,3,0x00}, {20,1,2,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 1234: sharp-axe Attack */
  {25,3,0,0x04}, {30,3,0,0x00}, {35,3,0,0x00}, {40,12,141,0x03}, {0,52,0,0x00}, {0,1,0,0x08},
  /* 1240: sharp-axe Death */
  {45,3,0,0x04}, {50,3,0,0x00}, {55,100,0,0x00}, {55,1,0,0x28},
  /* 1244: skeleton Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1246: skeleton Move */
  {0,2,3,0x04}, {10,2,3,0x00}, {10,1,3,0x00}, {25,2,2,0x00}, {25,3,3,0x00}, {0,1,2,0x00}, {0,2,3,0x00}, {40,2,3,0x00},
  {40,1,3,0x00}, {55,2,2,0x00}, {55,2,3,0x00}, {0,1,2,0x08},
  /* 1258: skeleton Attack */
  {15,4,0,0x04}, {30,4,0,0x00}, {45,4,145,0x03}, {60,4,0,0x00}, {0,18,0,0x00}, {0,1,0,0x08},
  /* 1264: skeleton Death */
  {5,3,0,0x04}, {20,3,0,0x00}, {35,3,0,0x00}, {50,3,0,0x00}, {65,3,0,0x00}, {65,1,0,0x28},
  /* 1270: daemon Still */
  {0,4,0,0x00}, {5,4,0,0x00}, {10,4,0,0x00}, {15,4,0,0x00},
  /* 1274: daemon Move */
  {0,1,3,0x04}, {0,1,3,0x00}, {5,1,2,0x00}, {5,1,3,0x00}, {10,1,2,0x00}, {10,1,3,0x00}, {10,1,3,0x00}, {15,1,2,0x00},
  {15,1,3,0x00}, {20,1,3,0x00}, {20,1,3,0x00}, {0,1,2,0x08},
  /* 1286: daemon Attack */
  {0,4,0,0x04}, {5,4,0,0x00}, {10,4,0,0x00}, {15,4,0,0x00}, {20,4,0,0x00}, {20,1,0,0x00}, {25,4,0,0x00}, {30,4,0,0x00},
  {35,4,0,0x00}, {40,4,0,0x01}, {45,4,0,0x00}, {0,1,0,0x08},
  /* 1298: daemon Death */
  {50,5,0,0x04}, {55,5,0,0x00}, {60,5,0,0x00}, {65,5,0,0x00}, {65,1,0,0x28},
  /* 1303: critter Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1305: critter Move */
  {0,2,2,0x04}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00},
  {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00}, {0,3,2,0x00},
  {0,1,0,0x08},
  /* 1322: critter Attack */
  {0,1,0,0x0D},
  /* 1323: critter Death */
  {5,200,0,0x04}, {5,1,0,0x28},
  /* 1325: building Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1327: building Train */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1329: building Research */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1331: building Upgrade */
  {1,4,0,0x00}, {1,1,0,0x00},
  /* 1333: oil-platform Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1335: oil-platform Active */
  {2,4,0,0x00}, {2,1,0,0x00},
  /* 1337: gold-mine Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1339: gold-mine Active */
  {1,4,0,0x00}, {1,1,0,0x00},
  /* 1341: human-guard-tower Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1343: human-guard-tower Attack */
  {0,59,0,0x05}, {0,1,0,0x08},
  /* 1345: orc-guard-tower Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1347: orc-guard-tower Attack */
  {0,59,0,0x05}, {0,1,0,0x08},
  /* 1349: human-cannon-tower Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1351: human-cannon-tower Attack */
  {0,150,0,0x05}, {0,1,0,0x08},
  /* 1353: orc-cannon-tower Still */
  {0,4,0,0x00}, {0,1,0,0x00},
  /* 1355: orc-cannon-tower Attack */
  {0,150,0,0x05}, {0,1,0,0x08},
};
const uint16_t w2_num_anim_steps = 1357;

#define X 0xFFFFu
const w2_anim_set w2_anim_sets[W2_NUM_ANIM_SETS] = {
  { "human-dead-body", {X,X,X,0,X,X,X,X,X,X,X}, {0,0,0,6,0,0,0,0,0,0,0} },
  { "orc-dead-body", {X,X,X,6,X,X,X,X,X,X,X}, {0,0,0,6,0,0,0,0,0,0,0} },
  { "dead-sea-body", {X,X,X,12,X,X,X,X,X,X,X}, {0,0,0,3,0,0,0,0,0,0,0} },
  { "destroyed-place", {X,X,X,15,X,X,X,X,X,X,X}, {0,0,0,3,0,0,0,0,0,0,0} },
  { "destroyed-place-water", {X,X,X,18,X,X,X,X,X,X,X}, {0,0,0,3,0,0,0,0,0,0,0} },
  { "footman", {21,23,35,41,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "grunt", {45,47,59,65,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "peasant", {69,71,83,90,94,101,X,X,X,X,X}, {2,12,7,4,7,7,0,0,0,0,0} },
  { "peon", {108,110,122,129,133,140,X,X,X,X,X}, {2,12,7,4,7,7,0,0,0,0,0} },
  { "ballista", {147,149,166,X,X,X,X,X,X,X,X}, {2,17,5,0,0,0,0,0,0,0,0} },
  { "catapult", {171,173,190,X,X,X,X,X,X,X,X}, {2,17,10,0,0,0,0,0,0,0,0} },
  { "knight", {200,202,212,218,X,X,X,X,X,X,X}, {2,10,6,6,0,0,0,0,0,0,0} },
  { "ogre", {224,226,238,244,X,X,X,X,X,X,X}, {2,12,6,6,0,0,0,0,0,0,0} },
  { "archer", {250,252,264,268,X,X,X,X,X,X,X}, {2,12,4,4,0,0,0,0,0,0,0} },
  { "axethrower", {272,274,286,292,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "mage", {296,298,310,316,X,X,324,X,X,X,X}, {2,12,6,8,0,0,6,0,0,0,0} },
  { "death-knight", {330,332,342,348,X,X,353,X,X,X,X}, {2,10,6,5,0,0,6,0,0,0,0} },
  { "paladin", {359,361,371,377,X,X,383,X,X,X,X}, {2,10,6,6,0,0,6,0,0,0,0} },
  { "ogre-mage", {389,391,403,409,X,X,415,X,X,X,X}, {2,12,6,6,0,0,6,0,0,0,0} },
  { "dwarves", {421,423,433,438,X,X,444,X,X,X,X}, {2,10,5,6,0,0,5,0,0,0,0} },
  { "goblin-sappers", {449,451,463,468,X,X,475,X,X,X,X}, {2,12,5,7,0,0,5,0,0,0,0} },
  { "ranger", {480,482,494,498,X,X,X,X,X,X,X}, {2,12,4,4,0,0,0,0,0,0,0} },
  { "female-hero", {502,504,516,520,X,X,X,X,X,X,X}, {2,12,4,4,0,0,0,0,0,0,0} },
  { "flying-angel", {524,528,540,573,X,X,X,X,X,X,X}, {4,12,33,7,0,0,0,0,0,0,0} },
  { "white-mage", {580,582,594,600,X,X,608,X,X,X,X}, {2,12,6,8,0,0,6,0,0,0,0} },
  { "human-oil-tanker", {614,616,632,635,X,X,X,X,X,X,X}, {2,16,3,3,0,0,0,0,0,0,0} },
  { "orc-oil-tanker", {638,640,656,659,X,X,X,X,X,X,X}, {2,16,3,3,0,0,0,0,0,0,0} },
  { "human-transport", {662,664,680,682,X,X,X,X,X,X,X}, {2,16,2,3,0,0,0,0,0,0,0} },
  { "orc-transport", {685,687,703,705,X,X,X,X,X,X,X}, {2,16,2,3,0,0,0,0,0,0,0} },
  { "elven-destroyer", {708,712,728,730,X,X,X,X,X,X,X}, {4,16,2,3,0,0,0,0,0,0,0} },
  { "troll-destroyer", {733,737,753,755,X,X,X,X,X,X,X}, {4,16,2,3,0,0,0,0,0,0,0} },
  { "battleship", {758,762,778,781,X,X,X,X,X,X,X}, {4,16,3,3,0,0,0,0,0,0,0} },
  { "fire-breeze", {784,788,796,827,X,X,X,X,X,X,X}, {4,8,31,6,0,0,0,0,0,0,0} },
  { "gnomish-submarine", {833,835,851,857,X,X,X,X,X,X,X}, {2,16,6,1,0,0,0,0,0,0,0} },
  { "orc-submarine", {858,860,876,882,X,X,X,X,X,X,X}, {2,16,6,1,0,0,0,0,0,0,0} },
  { "balloon", {883,887,897,898,X,X,X,X,X,X,X}, {4,10,1,1,0,0,0,0,0,0,0} },
  { "goblin-zeppelin", {899,903,913,914,X,X,X,X,X,X,X}, {4,10,1,1,0,0,0,0,0,0,0} },
  { "gryphon-rider", {915,919,931,964,X,X,X,X,X,X,X}, {4,12,33,7,0,0,0,0,0,0,0} },
  { "dragon", {971,975,983,1014,X,X,X,X,X,X,X}, {4,8,31,6,0,0,0,0,0,0,0} },
  { "knight-rider", {1020,1022,1032,1038,X,X,1044,X,X,X,X}, {2,10,6,6,0,0,6,0,0,0,0} },
  { "eye-of-vision", {1050,1052,1056,X,X,X,X,X,X,X,X}, {2,4,1,0,0,0,0,0,0,0,0} },
  { "arthor-literios", {1057,1059,1071,1077,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "quick-blade", {1081,1083,1095,1101,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "double-head", {1105,1107,1119,1125,X,X,1131,X,X,X,X}, {2,12,6,6,0,0,6,0,0,0,0} },
  { "wise-man", {1137,1139,1149,1155,X,X,1161,X,X,X,X}, {2,10,6,6,0,0,6,0,0,0,0} },
  { "ice-bringer", {1167,1169,1179,1185,X,X,X,X,X,X,X}, {2,10,6,5,0,0,0,0,0,0,0} },
  { "man-of-light", {1190,1192,1202,1208,X,X,1214,X,X,X,X}, {2,10,6,6,0,0,6,0,0,0,0} },
  { "sharp-axe", {1220,1222,1234,1240,X,X,X,X,X,X,X}, {2,12,6,4,0,0,0,0,0,0,0} },
  { "skeleton", {1244,1246,1258,1264,X,X,X,X,X,X,X}, {2,12,6,6,0,0,0,0,0,0,0} },
  { "daemon", {1270,1274,1286,1298,X,X,X,X,X,X,X}, {4,12,12,5,0,0,0,0,0,0,0} },
  { "critter", {1303,1305,1322,1323,X,X,X,X,X,X,X}, {2,17,1,2,0,0,0,0,0,0,0} },
  { "building", {1325,X,X,X,X,X,X,X,1327,1329,1331}, {2,0,0,0,0,0,0,0,2,2,2} },
  { "oil-platform", {1333,X,X,X,X,X,X,1335,X,X,X}, {2,0,0,0,0,0,0,2,0,0,0} },
  { "gold-mine", {1337,X,X,X,X,X,X,1339,X,X,X}, {2,0,0,0,0,0,0,2,0,0,0} },
  { "human-guard-tower", {1341,X,1343,X,X,X,X,X,X,X,X}, {2,0,2,0,0,0,0,0,0,0,0} },
  { "orc-guard-tower", {1345,X,1347,X,X,X,X,X,X,X,X}, {2,0,2,0,0,0,0,0,0,0,0} },
  { "human-cannon-tower", {1349,X,1351,X,X,X,X,X,X,X,X}, {2,0,2,0,0,0,0,0,0,0,0} },
  { "orc-cannon-tower", {1353,X,1355,X,X,X,X,X,X,X,X}, {2,0,2,0,0,0,0,0,0,0,0} },
};
#undef X

const w2_missile w2_missiles[W2_NUM_MISSILES] = {
  { "lightning", 324, 32, 32, 30, 9, W2MC_POINT_TO_POINT_WITH_HIT, 16, 1, 1, 0, 0, N, N, 50 },
  { "griffon-hammer", 325, 32, 32, 15, 9, W2MC_POINT_TO_POINT_BOUNCE, 16, 1, 2, 2, 3, 23, 146, 50 },
  { "dragon-breath", 326, 32, 32, 5, 9, W2MC_POINT_TO_POINT_BOUNCE, 16, 1, 2, 2, 3, 23, 146, 70 },
  { "fireball", 327, 32, 32, 5, 9, W2MC_POINT_TO_POINT_BOUNCE, 16, 1, 2, 2, 5, 23, 146, 50 },
  { "flame-shield", 328, 32, 48, 6, 1, W2MC_FLAME_SHIELD, 4, 1, 1, 0, 0, N, N, 50 },
  { "blizzard", 329, 32, 32, 4, 1, W2MC_POINT_TO_POINT_WITH_HIT, 16, 1, 1, 0, 0, N, 146, 100 },
  { "death-and-decay", 330, 32, 32, 8, 1, W2MC_STAY, 0, 1, 1, 0, 0, N, N, 100 },
  { "big-cannon", 331, 16, 16, 20, 9, W2MC_POINT_TO_POINT, 16, 1, 2, 4, 0, 26, 22, 50 },
  { "exorcism", 332, 48, 48, 10, 1, W2MC_POINT_TO_POINT, 16, 1, 1, 0, 0, N, N, 50 },
  { "heal-effect", 333, 48, 48, 10, 1, W2MC_STAY, 16, 1, 1, 0, 0, N, N, 50 },
  { "touch-of-death", 334, 32, 32, 30, 9, W2MC_POINT_TO_POINT_WITH_HIT, 16, 1, 1, 0, 0, N, N, 50 },
  { "rune", 335, 16, 16, 4, 1, W2MC_LAND_MINE, 16, 5, 1, 0, 0, 23, N, 20 },
  { "whirlwind", 336, 56, 56, 4, 1, W2MC_WHIRLWIND, 2, 1, 2, 0, 0, N, N, 50 },
  { "catapult-rock", 337, 32, 32, 15, 9, W2MC_POINT_TO_POINT, 8, 1, 2, 4, 0, 21, 22, 50 },
  { "ballista-bolt", 338, 64, 64, 5, 9, W2MC_POINT_TO_POINT, 8, 1, 2, 4, 0, 21, 22, 50 },
  { "arrow", 339, 40, 40, 5, 9, W2MC_POINT_TO_POINT, 32, 1, 0, 0, 0, N, 147, 50 },
  { "axe", 340, 32, 32, 15, 9, W2MC_POINT_TO_POINT, 32, 1, 0, 0, 0, N, 147, 50 },
  { "submarine-missile", 341, 40, 40, 5, 9, W2MC_POINT_TO_POINT, 16, 1, 1, 0, 0, 21, 22, 50 },
  { "turtle-missile", 342, 40, 40, 5, 9, W2MC_POINT_TO_POINT, 16, 1, 1, 0, 0, 21, 22, 50 },
  { "small-fire", 343, 32, 48, 6, 1, W2MC_FIRE, 16, 2, 1, 0, 0, N, N, 45 },
  { "big-fire", 344, 48, 48, 10, 1, W2MC_FIRE, 16, 2, 1, 0, 0, N, N, 45 },
  { "impact", 345, 48, 48, 10, 1, W2MC_STAY, 16, 1, 1, 0, 0, N, N, 50 },
  { "normal-spell", 346, 32, 32, 6, 1, W2MC_STAY, 0, 5, 1, 0, 0, N, N, 50 },
  { "explosion", 347, 64, 64, 20, 1, W2MC_STAY, 16, 1, 1, 0, 0, N, N, 50 },
  { "small-cannon", 348, 32, 32, 15, 9, W2MC_POINT_TO_POINT, 22, 1, 2, 3, 0, 26, 22, 50 },
  { "cannon-explosion", 349, 32, 32, 4, 1, W2MC_STAY, 16, 2, 1, 0, 0, N, N, 50 },
  { "cannon-tower-explosion", 350, 32, 32, 4, 1, W2MC_STAY, 16, 2, 1, 0, 0, N, N, 50 },
  { "daemon-fire", 351, 32, 32, 15, 9, W2MC_POINT_TO_POINT, 16, 1, 1, 0, 0, N, 146, 70 },
  { "green-cross", 352, 32, 32, 4, 1, W2MC_CYCLE_ONCE, 16, 1, 1, 0, 0, N, N, 150 },
  { "none", 0, 32, 32, 0, 1, W2MC_NONE, 16, 1, 1, 0, 0, N, N, 50 },
  { "death-coil", 334, 32, 32, 30, 9, W2MC_DEATH_COIL, 16, 1, 1, 0, 0, N, N, 50 },
};

const w2_upgrade w2_upgrades[W2_NUM_UPGRADES] = {
  { "Upgrade Sword Strength 1", 800, 0, 0, 200, 117, W2_RACE_HUMAN, 0x52, 1, N, 0x00000004ul },
  { "Upgrade Sword Strength 2", 2400, 0, 0, 250, 118, W2_RACE_HUMAN, 0x52, 1, N, 0x00000008ul },
  { "Upgrade Axe Strength 1", 500, 100, 0, 200, 120, W2_RACE_ORC, 0x53, 1, N, 0x00000004ul },
  { "Upgrade Axe Strength 2", 1500, 300, 0, 250, 121, W2_RACE_ORC, 0x53, 1, N, 0x00000008ul },
  { "Upgrade Arrow Strength 1", 300, 300, 0, 200, 125, W2_RACE_HUMAN, 0x4C, 0, N, 0x00000001ul },
  { "Upgrade Arrow Strength 2", 900, 500, 0, 250, 126, W2_RACE_HUMAN, 0x4C, 0, N, 0x00000001ul },
  { "Upgrade Spear Strength 1", 300, 300, 0, 200, 128, W2_RACE_ORC, 0x4D, 0, N, 0x00000001ul },
  { "Upgrade Spear Strength 2", 900, 500, 0, 250, 129, W2_RACE_ORC, 0x4D, 0, N, 0x00000002ul },
  { "Upgrade Human Shield Strength 1", 300, 300, 0, 200, 165, W2_RACE_HUMAN, 0x52, 2, N, 0x00000010ul },
  { "Upgrade Human Shield Strength 2", 900, 500, 0, 250, 166, W2_RACE_HUMAN, 0x52, 2, N, 0x00000020ul },
  { "Upgrade Orc Shield Strength 1", 300, 300, 0, 200, 168, W2_RACE_ORC, 0x53, 2, N, 0x00000010ul },
  { "Upgrade Orc Shield Strength 2", 900, 500, 0, 250, 169, W2_RACE_ORC, 0x53, 2, N, 0x00000020ul },
  { "Upgrade Human Ship Attack 1", 700, 100, 1000, 200, 145, W2_RACE_HUMAN, 0x4E, 3, N, 0x00000040ul },
  { "Upgrade Human Ship Attack 2", 2000, 250, 3000, 250, 146, W2_RACE_HUMAN, 0x4E, 3, N, 0x00000080ul },
  { "Upgrade Orc Ship Attack 1", 700, 100, 1000, 200, 148, W2_RACE_ORC, 0x4F, 3, N, 0x00000040ul },
  { "Upgrade Orc Ship Attack 2", 2000, 250, 3000, 250, 149, W2_RACE_ORC, 0x4F, 3, N, 0x00000080ul },
  { "Upgrade Human Ship Armor 1", 500, 500, 0, 200, 154, W2_RACE_HUMAN, 0x4E, 4, N, 0x00000100ul },
  { "Upgrade Human Ship Armor 2", 1500, 900, 0, 250, 155, W2_RACE_HUMAN, 0x4E, 4, N, 0x00000200ul },
  { "Upgrade Orc Ship Armor 1", 500, 500, 0, 200, 151, W2_RACE_ORC, 0x4F, 4, N, 0x00000100ul },
  { "Upgrade Orc Ship Armor 2", 1500, 900, 0, 250, 152, W2_RACE_ORC, 0x4F, 4, N, 0x00000200ul },
  { "Upgrade Catapult Strength 1", 1500, 0, 0, 250, 138, W2_RACE_ORC, 0x53, 6, N, 0x00001000ul },
  { "Upgrade Catapult Strength 2", 4000, 0, 0, 250, 139, W2_RACE_ORC, 0x53, 6, N, 0x00002000ul },
  { "Upgrade Ballista Strength 1", 1500, 0, 0, 250, 140, W2_RACE_HUMAN, 0x52, 6, N, 0x00001000ul },
  { "Upgrade Ballista Strength 2", 4000, 0, 0, 250, 141, W2_RACE_HUMAN, 0x52, 6, N, 0x00002000ul },
  { "Elven Ranger Training", 1500, 0, 0, 250, 6, W2_RACE_HUMAN, 0x4C, 7, N, 0x00010000ul },
  { "Research Longbow", 2000, 0, 0, 250, 132, W2_RACE_HUMAN, 0x4C, 8, N, 0x00020000ul },
  { "Ranger Scouting", 1500, 0, 0, 250, 133, W2_RACE_HUMAN, 0x4C, 9, N, 0x00040000ul },
  { "Ranger Marksmanship", 2500, 0, 0, 250, 134, W2_RACE_HUMAN, 0x4C, 10, N, 0x00080000ul },
  { "Troll Berserker Training", 1500, 0, 0, 250, 7, W2_RACE_ORC, 0x4D, 7, N, 0x00010000ul },
  { "Research Lighter Axes", 2000, 0, 0, 250, 135, W2_RACE_ORC, 0x4D, 8, N, 0x00020000ul },
  { "Berserker Scouting", 1500, 0, 0, 250, 136, W2_RACE_ORC, 0x4D, 9, N, 0x00040000ul },
  { "Berserker Regeneration", 3000, 0, 0, 250, 137, W2_RACE_ORC, 0x4D, 10, N, 0x00080000ul },
  { "Upgrade Ogres to Ogre-Mages", 1000, 0, 0, 250, 11, W2_RACE_ORC, 0x3F, 20, N, 0x00100000ul },
  { "Upgrade Knights to Paladins", 1000, 0, 0, 250, 10, W2_RACE_HUMAN, 0x3E, 20, N, 0x00100000ul },
  { "Spell - Holy Vision", 0, 0, 0, 0, 106, W2_RACE_HUMAN, N, 0, 0, 0x00000001ul },
  { "Spell - Healing", 1000, 0, 0, 200, 107, W2_RACE_HUMAN, 0x3E, 1, 1, 0x00000002ul },
  { "Spell - Exorcism", 2000, 0, 0, 200, 110, W2_RACE_HUMAN, 0x3E, 3, 2, 0x00000008ul },
  { "Spell - Flame Shield", 1000, 0, 0, 100, 100, W2_RACE_HUMAN, 0x50, 4, 5, 0x00000010ul },
  { "Spell - Fireball", 0, 0, 0, 0, 101, W2_RACE_HUMAN, N, 5, 3, 0x00000020ul },
  { "Spell - Slow", 500, 0, 0, 100, 94, W2_RACE_HUMAN, 0x50, 6, 4, 0x00000040ul },
  { "Spell - Invisibility", 2500, 0, 0, 200, 95, W2_RACE_HUMAN, 0x50, 7, 6, 0x00000080ul },
  { "Spell - Polymorph", 2000, 0, 0, 200, 115, W2_RACE_HUMAN, 0x50, 8, 7, 0x00000100ul },
  { "Spell - Blizzard", 2000, 0, 0, 200, 105, W2_RACE_HUMAN, 0x50, 9, 8, 0x00000200ul },
  { "Spell - Eye of Kilrogg", 0, 0, 0, 0, 111, W2_RACE_ORC, N, 10, 9, 0x00000400ul },
  { "Spell - Bloodlust", 1000, 0, 0, 100, 112, W2_RACE_ORC, 0x3F, 11, 10, 0x00000800ul },
  { "Spell - Raise Dead", 1500, 0, 0, 100, 114, W2_RACE_ORC, 0x51, 13, 14, 0x00002000ul },
  { "Spell - Death Coil", 0, 0, 0, 100, 103, W2_RACE_ORC, N, 14, 12, 0x00004000ul },
  { "Spell - Whirlwind", 1500, 0, 0, 150, 104, W2_RACE_ORC, 0x51, 15, 15, 0x00008000ul },
  { "Spell - Haste", 500, 0, 0, 100, 96, W2_RACE_ORC, 0x51, 16, 13, 0x00010000ul },
  { "Spell - Unholy Armor", 2500, 0, 0, 200, 98, W2_RACE_ORC, 0x51, 17, 16, 0x00020000ul },
  { "Spell - Runes", 1000, 0, 0, 150, 97, W2_RACE_ORC, 0x3F, 18, 11, 0x00040000ul },
  { "Spell - Death and Decay", 2000, 0, 0, 200, 108, W2_RACE_ORC, 0x51, 19, 17, 0x00080000ul },
};

const w2_modifier w2_modifiers[] = {
  { W2UP_SWORD1, W2U_FOOTMAN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_KNIGHT, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_PALADIN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_DWARVES, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_ARTHOR_LITERIOS, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_WISE_MAN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_MAN_OF_LIGHT, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_KNIGHT_RIDER, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD1, W2U_ATTACK_PEASANT, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_FOOTMAN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_KNIGHT, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_PALADIN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_DWARVES, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_ARTHOR_LITERIOS, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_WISE_MAN, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_MAN_OF_LIGHT, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_KNIGHT_RIDER, W2_STAT_PIERCING, 2 },
  { W2UP_SWORD2, W2U_ATTACK_PEASANT, W2_STAT_PIERCING, 2 },
  { W2UP_ARROW1, W2U_ARCHER, W2_STAT_PIERCING, 1 },
  { W2UP_ARROW1, W2U_RANGER, W2_STAT_PIERCING, 1 },
  { W2UP_ARROW1, W2U_FEMALE_HERO, W2_STAT_PIERCING, 1 },
  { W2UP_ARROW2, W2U_ARCHER, W2_STAT_PIERCING, 1 },
  { W2UP_ARROW2, W2U_RANGER, W2_STAT_PIERCING, 1 },
  { W2UP_ARROW2, W2U_FEMALE_HERO, W2_STAT_PIERCING, 1 },
  { W2UP_HUMAN_SHIELD1, W2U_FOOTMAN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_KNIGHT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_PALADIN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_DWARVES, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_ARTHOR_LITERIOS, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_WISE_MAN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_MAN_OF_LIGHT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_KNIGHT_RIDER, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD1, W2U_ATTACK_PEASANT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_FOOTMAN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_KNIGHT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_PALADIN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_DWARVES, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_ARTHOR_LITERIOS, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_WISE_MAN, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_MAN_OF_LIGHT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_KNIGHT_RIDER, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIELD2, W2U_ATTACK_PEASANT, W2_STAT_ARMOR, 2 },
  { W2UP_HUMAN_SHIP_CANNON1, W2U_HUMAN_DESTROYER, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_CANNON1, W2U_BATTLESHIP, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_CANNON1, W2U_HUMAN_SUBMARINE, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_CANNON2, W2U_HUMAN_DESTROYER, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_CANNON2, W2U_BATTLESHIP, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_CANNON2, W2U_HUMAN_SUBMARINE, W2_STAT_PIERCING, 5 },
  { W2UP_HUMAN_SHIP_ARMOR1, W2U_HUMAN_DESTROYER, W2_STAT_ARMOR, 5 },
  { W2UP_HUMAN_SHIP_ARMOR1, W2U_BATTLESHIP, W2_STAT_ARMOR, 5 },
  { W2UP_HUMAN_SHIP_ARMOR1, W2U_HUMAN_TRANSPORT, W2_STAT_ARMOR, 5 },
  { W2UP_HUMAN_SHIP_ARMOR2, W2U_HUMAN_DESTROYER, W2_STAT_ARMOR, 5 },
  { W2UP_HUMAN_SHIP_ARMOR2, W2U_BATTLESHIP, W2_STAT_ARMOR, 5 },
  { W2UP_HUMAN_SHIP_ARMOR2, W2U_HUMAN_TRANSPORT, W2_STAT_ARMOR, 5 },
  { W2UP_BALLISTA1, W2U_BALLISTA, W2_STAT_PIERCING, 15 },
  { W2UP_BALLISTA2, W2U_BALLISTA, W2_STAT_PIERCING, 15 },
  { W2UP_RANGER, W2U_ARCHER, W2_STAT_CONVERT, 18 },
  { W2UP_LONGBOW, W2U_ARCHER, W2_STAT_SIGHT, 1 },
  { W2UP_LONGBOW, W2U_ARCHER, W2_STAT_RANGE, 1 },
  { W2UP_LONGBOW, W2U_RANGER, W2_STAT_SIGHT, 1 },
  { W2UP_LONGBOW, W2U_RANGER, W2_STAT_RANGE, 1 },
  { W2UP_RANGER_SCOUTING, W2U_ARCHER, W2_STAT_SIGHT, 3 },
  { W2UP_RANGER_SCOUTING, W2U_RANGER, W2_STAT_SIGHT, 3 },
  { W2UP_RANGER_MARKSMANSHIP, W2U_ARCHER, W2_STAT_PIERCING, 3 },
  { W2UP_RANGER_MARKSMANSHIP, W2U_RANGER, W2_STAT_PIERCING, 3 },
  { W2UP_PALADIN, W2U_KNIGHT, W2_STAT_CONVERT, 12 },
  { W2UP_HOLY_VISION, W2U_PALADIN, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_HEALING, W2U_PALADIN, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_EXORCISM, W2U_PALADIN, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_FLAME_SHIELD, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_FLAME_SHIELD, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_FIREBALL, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_FIREBALL, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_SLOW, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_SLOW, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_INVISIBILITY, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_INVISIBILITY, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_POLYMORPH, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_POLYMORPH, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_BLIZZARD, W2U_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_BLIZZARD, W2U_WHITE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_BATTLE_AXE1, W2U_GRUNT, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_OGRE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_OGRE_MAGE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_GOBLIN_SAPPERS, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_QUICK_BLADE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_BEAST_CRY, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_FAD_MAN, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE1, W2U_DOUBLE_HEAD, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_GRUNT, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_OGRE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_OGRE_MAGE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_GOBLIN_SAPPERS, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_QUICK_BLADE, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_BEAST_CRY, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_FAD_MAN, W2_STAT_PIERCING, 2 },
  { W2UP_BATTLE_AXE2, W2U_DOUBLE_HEAD, W2_STAT_PIERCING, 2 },
  { W2UP_THROWING_AXE1, W2U_AXETHROWER, W2_STAT_PIERCING, 1 },
  { W2UP_THROWING_AXE1, W2U_BERSERKER, W2_STAT_PIERCING, 1 },
  { W2UP_THROWING_AXE1, W2U_SHARP_AXE, W2_STAT_PIERCING, 1 },
  { W2UP_THROWING_AXE2, W2U_AXETHROWER, W2_STAT_PIERCING, 1 },
  { W2UP_THROWING_AXE2, W2U_BERSERKER, W2_STAT_PIERCING, 1 },
  { W2UP_THROWING_AXE2, W2U_SHARP_AXE, W2_STAT_PIERCING, 1 },
  { W2UP_ORC_SHIELD1, W2U_GRUNT, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_OGRE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_OGRE_MAGE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_GOBLIN_SAPPERS, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_QUICK_BLADE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_BEAST_CRY, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_FAD_MAN, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD1, W2U_DOUBLE_HEAD, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_GRUNT, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_OGRE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_OGRE_MAGE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_GOBLIN_SAPPERS, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_QUICK_BLADE, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_BEAST_CRY, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_FAD_MAN, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIELD2, W2U_DOUBLE_HEAD, W2_STAT_ARMOR, 2 },
  { W2UP_ORC_SHIP_CANNON1, W2U_ORC_DESTROYER, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_CANNON1, W2U_OGRE_JUGGERNAUGHT, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_CANNON1, W2U_ORC_SUBMARINE, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_CANNON2, W2U_ORC_DESTROYER, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_CANNON2, W2U_OGRE_JUGGERNAUGHT, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_CANNON2, W2U_ORC_SUBMARINE, W2_STAT_PIERCING, 5 },
  { W2UP_ORC_SHIP_ARMOR1, W2U_ORC_DESTROYER, W2_STAT_ARMOR, 5 },
  { W2UP_ORC_SHIP_ARMOR1, W2U_OGRE_JUGGERNAUGHT, W2_STAT_ARMOR, 5 },
  { W2UP_ORC_SHIP_ARMOR1, W2U_ORC_TRANSPORT, W2_STAT_ARMOR, 5 },
  { W2UP_ORC_SHIP_ARMOR2, W2U_ORC_DESTROYER, W2_STAT_ARMOR, 5 },
  { W2UP_ORC_SHIP_ARMOR2, W2U_OGRE_JUGGERNAUGHT, W2_STAT_ARMOR, 5 },
  { W2UP_ORC_SHIP_ARMOR2, W2U_ORC_TRANSPORT, W2_STAT_ARMOR, 5 },
  { W2UP_CATAPULT1, W2U_CATAPULT, W2_STAT_PIERCING, 15 },
  { W2UP_CATAPULT2, W2U_CATAPULT, W2_STAT_PIERCING, 15 },
  { W2UP_BERSERKER, W2U_AXETHROWER, W2_STAT_CONVERT, 19 },
  { W2UP_LIGHT_AXES, W2U_AXETHROWER, W2_STAT_RANGE, 1 },
  { W2UP_LIGHT_AXES, W2U_BERSERKER, W2_STAT_RANGE, 1 },
  { W2UP_BERSERKER_SCOUTING, W2U_AXETHROWER, W2_STAT_SIGHT, 3 },
  { W2UP_BERSERKER_SCOUTING, W2U_BERSERKER, W2_STAT_SIGHT, 3 },
  { W2UP_BERSERKER_REGENERATION, W2U_AXETHROWER, W2_STAT_REGEN, 1 },
  { W2UP_BERSERKER_REGENERATION, W2U_BERSERKER, W2_STAT_REGEN, 1 },
  { W2UP_OGRE_MAGE, W2U_OGRE, W2_STAT_CONVERT, 13 },
  { W2UP_EYE_OF_KILROGG, W2U_OGRE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_BLOODLUST, W2U_OGRE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_RUNES, W2U_OGRE_MAGE, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_RAISE_DEAD, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_RAISE_DEAD, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_RAISE_DEAD, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_COIL, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_COIL, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_COIL, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_WHIRLWIND, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_WHIRLWIND, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_WHIRLWIND, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_HASTE, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_HASTE, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_HASTE, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_UNHOLY_ARMOR, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_UNHOLY_ARMOR, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_UNHOLY_ARMOR, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_AND_DECAY, W2U_DEATH_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_AND_DECAY, W2U_ICE_BRINGER, W2_STAT_ENABLE_SPELL, 0 },
  { W2UP_DEATH_AND_DECAY, W2U_EVIL_KNIGHT, W2_STAT_ENABLE_SPELL, 0 },
};
const uint16_t w2_num_modifiers = 162;

const w2_dependency w2_dependencies[] = {
  { W2_DEP_UNIT, 8, 0, W2_DEP_UNIT, 76 },  /* unit-archer <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 4, 0, W2_DEP_UNIT, 82 },  /* unit-ballista <- unit-human-blacksmith */
  { W2_DEP_UNIT, 4, 0, W2_DEP_UNIT, 76 },  /* unit-ballista <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 6, 0, W2_DEP_UNIT, 66 },  /* unit-knight <- unit-stables */
  { W2_DEP_UNIT, 6, 0, W2_DEP_UNIT, 82 },  /* unit-knight <- unit-human-blacksmith */
  { W2_DEP_UNIT, 18, 0, W2_DEP_UPGRADE, 24 },  /* unit-ranger <- upgrade-ranger */
  { W2_DEP_UNIT, 18, 0, W2_DEP_UNIT, 76 },  /* unit-ranger <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 12, 0, W2_DEP_UPGRADE, 33 },  /* unit-paladin <- upgrade-paladin */
  { W2_DEP_UNIT, 12, 0, W2_DEP_UNIT, 66 },  /* unit-paladin <- unit-stables */
  { W2_DEP_UNIT, 12, 0, W2_DEP_UNIT, 82 },  /* unit-paladin <- unit-human-blacksmith */
  { W2_DEP_UNIT, 38, 0, W2_DEP_UNIT, 68 },  /* unit-human-submarine <- unit-inventor */
  { W2_DEP_UNIT, 28, 0, W2_DEP_UNIT, 78 },  /* unit-human-transport <- unit-human-foundry */
  { W2_DEP_UNIT, 32, 0, W2_DEP_UNIT, 78 },  /* unit-battleship <- unit-human-foundry */
  { W2_DEP_UNIT, 40, 0, W2_DEP_UNIT, 76 },  /* unit-balloon <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 96, 0, W2_DEP_UNIT, 76 },  /* unit-human-guard-tower <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 98, 0, W2_DEP_UNIT, 82 },  /* unit-human-cannon-tower <- unit-human-blacksmith */
  { W2_DEP_UNIT, 72, 0, W2_DEP_UNIT, 76 },  /* unit-human-shipyard <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 78, 0, W2_DEP_UNIT, 72 },  /* unit-human-foundry <- unit-human-shipyard */
  { W2_DEP_UNIT, 84, 0, W2_DEP_UNIT, 72 },  /* unit-human-refinery <- unit-human-shipyard */
  { W2_DEP_UNIT, 88, 0, W2_DEP_UNIT, 60 },  /* unit-keep <- unit-human-barracks */
  { W2_DEP_UNIT, 68, 0, W2_DEP_UNIT, 88 },  /* unit-inventor <- unit-keep */
  { W2_DEP_UNIT, 68, 1, W2_DEP_UNIT, 90 },  /* unit-inventor <- unit-castle */
  { W2_DEP_UNIT, 66, 0, W2_DEP_UNIT, 88 },  /* unit-stables <- unit-keep */
  { W2_DEP_UNIT, 66, 1, W2_DEP_UNIT, 90 },  /* unit-stables <- unit-castle */
  { W2_DEP_UNIT, 90, 0, W2_DEP_UNIT, 66 },  /* unit-castle <- unit-stables */
  { W2_DEP_UNIT, 90, 0, W2_DEP_UNIT, 82 },  /* unit-castle <- unit-human-blacksmith */
  { W2_DEP_UNIT, 90, 0, W2_DEP_UNIT, 76 },  /* unit-castle <- unit-elven-lumber-mill */
  { W2_DEP_UNIT, 80, 0, W2_DEP_UNIT, 90 },  /* unit-mage-tower <- unit-castle */
  { W2_DEP_UNIT, 62, 0, W2_DEP_UNIT, 90 },  /* unit-church <- unit-castle */
  { W2_DEP_UNIT, 70, 0, W2_DEP_UNIT, 90 },  /* unit-gryphon-aviary <- unit-castle */
  { W2_DEP_UPGRADE, 1, 0, W2_DEP_UPGRADE, 0 },  /* upgrade-sword2 <- upgrade-sword1 */
  { W2_DEP_UPGRADE, 5, 0, W2_DEP_UPGRADE, 4 },  /* upgrade-arrow2 <- upgrade-arrow1 */
  { W2_DEP_UPGRADE, 9, 0, W2_DEP_UPGRADE, 8 },  /* upgrade-human-shield2 <- upgrade-human-shield1 */
  { W2_DEP_UPGRADE, 23, 0, W2_DEP_UPGRADE, 22 },  /* upgrade-ballista2 <- upgrade-ballista1 */
  { W2_DEP_UPGRADE, 13, 0, W2_DEP_UPGRADE, 12 },  /* upgrade-human-ship-cannon2 <- upgrade-human-ship-cannon1 */
  { W2_DEP_UPGRADE, 17, 0, W2_DEP_UPGRADE, 16 },  /* upgrade-human-ship-armor2 <- upgrade-human-ship-armor1 */
  { W2_DEP_UPGRADE, 24, 0, W2_DEP_UNIT, 88 },  /* upgrade-ranger <- unit-keep */
  { W2_DEP_UPGRADE, 24, 1, W2_DEP_UNIT, 90 },  /* upgrade-ranger <- unit-castle */
  { W2_DEP_UPGRADE, 25, 0, W2_DEP_UNIT, 88 },  /* upgrade-longbow <- unit-keep */
  { W2_DEP_UPGRADE, 25, 0, W2_DEP_UPGRADE, 24 },  /* upgrade-longbow <- upgrade-ranger */
  { W2_DEP_UPGRADE, 25, 1, W2_DEP_UNIT, 90 },  /* upgrade-longbow <- unit-castle */
  { W2_DEP_UPGRADE, 25, 1, W2_DEP_UPGRADE, 24 },  /* upgrade-longbow <- upgrade-ranger */
  { W2_DEP_UPGRADE, 26, 0, W2_DEP_UNIT, 88 },  /* upgrade-ranger-scouting <- unit-keep */
  { W2_DEP_UPGRADE, 26, 0, W2_DEP_UPGRADE, 24 },  /* upgrade-ranger-scouting <- upgrade-ranger */
  { W2_DEP_UPGRADE, 26, 1, W2_DEP_UNIT, 90 },  /* upgrade-ranger-scouting <- unit-castle */
  { W2_DEP_UPGRADE, 26, 1, W2_DEP_UPGRADE, 24 },  /* upgrade-ranger-scouting <- upgrade-ranger */
  { W2_DEP_UPGRADE, 27, 0, W2_DEP_UNIT, 88 },  /* upgrade-ranger-marksmanship <- unit-keep */
  { W2_DEP_UPGRADE, 27, 0, W2_DEP_UPGRADE, 24 },  /* upgrade-ranger-marksmanship <- upgrade-ranger */
  { W2_DEP_UPGRADE, 27, 1, W2_DEP_UNIT, 90 },  /* upgrade-ranger-marksmanship <- unit-castle */
  { W2_DEP_UPGRADE, 27, 1, W2_DEP_UPGRADE, 24 },  /* upgrade-ranger-marksmanship <- upgrade-ranger */
  { W2_DEP_UPGRADE, 35, 0, W2_DEP_UPGRADE, 33 },  /* upgrade-healing <- upgrade-paladin */
  { W2_DEP_UPGRADE, 36, 0, W2_DEP_UPGRADE, 33 },  /* upgrade-exorcism <- upgrade-paladin */
  { W2_DEP_UNIT, 9, 0, W2_DEP_UNIT, 77 },  /* unit-axethrower <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 5, 0, W2_DEP_UNIT, 83 },  /* unit-catapult <- unit-orc-blacksmith */
  { W2_DEP_UNIT, 5, 0, W2_DEP_UNIT, 77 },  /* unit-catapult <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 19, 0, W2_DEP_UPGRADE, 28 },  /* unit-berserker <- upgrade-berserker */
  { W2_DEP_UNIT, 19, 0, W2_DEP_UNIT, 77 },  /* unit-berserker <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 7, 0, W2_DEP_UNIT, 67 },  /* unit-ogre <- unit-ogre-mound */
  { W2_DEP_UNIT, 7, 0, W2_DEP_UNIT, 83 },  /* unit-ogre <- unit-orc-blacksmith */
  { W2_DEP_UNIT, 13, 0, W2_DEP_UPGRADE, 32 },  /* unit-ogre-mage <- upgrade-ogre-mage */
  { W2_DEP_UNIT, 13, 0, W2_DEP_UNIT, 67 },  /* unit-ogre-mage <- unit-ogre-mound */
  { W2_DEP_UNIT, 13, 0, W2_DEP_UNIT, 83 },  /* unit-ogre-mage <- unit-orc-blacksmith */
  { W2_DEP_UNIT, 39, 0, W2_DEP_UNIT, 69 },  /* unit-orc-submarine <- unit-alchemist */
  { W2_DEP_UNIT, 29, 0, W2_DEP_UNIT, 79 },  /* unit-orc-transport <- unit-orc-foundry */
  { W2_DEP_UNIT, 33, 0, W2_DEP_UNIT, 79 },  /* unit-ogre-juggernaught <- unit-orc-foundry */
  { W2_DEP_UNIT, 41, 0, W2_DEP_UNIT, 77 },  /* unit-zeppelin <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 97, 0, W2_DEP_UNIT, 77 },  /* unit-orc-guard-tower <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 99, 0, W2_DEP_UNIT, 83 },  /* unit-orc-cannon-tower <- unit-orc-blacksmith */
  { W2_DEP_UNIT, 73, 0, W2_DEP_UNIT, 77 },  /* unit-orc-shipyard <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 79, 0, W2_DEP_UNIT, 73 },  /* unit-orc-foundry <- unit-orc-shipyard */
  { W2_DEP_UNIT, 85, 0, W2_DEP_UNIT, 73 },  /* unit-orc-refinery <- unit-orc-shipyard */
  { W2_DEP_UNIT, 89, 0, W2_DEP_UNIT, 61 },  /* unit-stronghold <- unit-orc-barracks */
  { W2_DEP_UNIT, 69, 0, W2_DEP_UNIT, 89 },  /* unit-alchemist <- unit-stronghold */
  { W2_DEP_UNIT, 69, 1, W2_DEP_UNIT, 91 },  /* unit-alchemist <- unit-fortress */
  { W2_DEP_UNIT, 67, 0, W2_DEP_UNIT, 89 },  /* unit-ogre-mound <- unit-stronghold */
  { W2_DEP_UNIT, 67, 1, W2_DEP_UNIT, 91 },  /* unit-ogre-mound <- unit-fortress */
  { W2_DEP_UNIT, 91, 0, W2_DEP_UNIT, 67 },  /* unit-fortress <- unit-ogre-mound */
  { W2_DEP_UNIT, 91, 0, W2_DEP_UNIT, 83 },  /* unit-fortress <- unit-orc-blacksmith */
  { W2_DEP_UNIT, 91, 0, W2_DEP_UNIT, 77 },  /* unit-fortress <- unit-troll-lumber-mill */
  { W2_DEP_UNIT, 63, 0, W2_DEP_UNIT, 91 },  /* unit-altar-of-storms <- unit-fortress */
  { W2_DEP_UNIT, 81, 0, W2_DEP_UNIT, 91 },  /* unit-temple-of-the-damned <- unit-fortress */
  { W2_DEP_UNIT, 71, 0, W2_DEP_UNIT, 91 },  /* unit-dragon-roost <- unit-fortress */
  { W2_DEP_UPGRADE, 3, 0, W2_DEP_UPGRADE, 2 },  /* upgrade-battle-axe2 <- upgrade-battle-axe1 */
  { W2_DEP_UPGRADE, 7, 0, W2_DEP_UPGRADE, 6 },  /* upgrade-throwing-axe2 <- upgrade-throwing-axe1 */
  { W2_DEP_UPGRADE, 11, 0, W2_DEP_UPGRADE, 10 },  /* upgrade-orc-shield2 <- upgrade-orc-shield1 */
  { W2_DEP_UPGRADE, 21, 0, W2_DEP_UPGRADE, 20 },  /* upgrade-catapult2 <- upgrade-catapult1 */
  { W2_DEP_UPGRADE, 15, 0, W2_DEP_UPGRADE, 14 },  /* upgrade-orc-ship-cannon2 <- upgrade-orc-ship-cannon1 */
  { W2_DEP_UPGRADE, 19, 0, W2_DEP_UPGRADE, 18 },  /* upgrade-orc-ship-armor2 <- upgrade-orc-ship-armor1 */
  { W2_DEP_UPGRADE, 28, 0, W2_DEP_UNIT, 89 },  /* upgrade-berserker <- unit-stronghold */
  { W2_DEP_UPGRADE, 28, 1, W2_DEP_UNIT, 91 },  /* upgrade-berserker <- unit-fortress */
  { W2_DEP_UPGRADE, 30, 0, W2_DEP_UNIT, 89 },  /* upgrade-berserker-scouting <- unit-stronghold */
  { W2_DEP_UPGRADE, 30, 0, W2_DEP_UPGRADE, 28 },  /* upgrade-berserker-scouting <- upgrade-berserker */
  { W2_DEP_UPGRADE, 30, 1, W2_DEP_UNIT, 91 },  /* upgrade-berserker-scouting <- unit-fortress */
  { W2_DEP_UPGRADE, 30, 1, W2_DEP_UPGRADE, 28 },  /* upgrade-berserker-scouting <- upgrade-berserker */
  { W2_DEP_UPGRADE, 31, 0, W2_DEP_UNIT, 89 },  /* upgrade-berserker-regeneration <- unit-stronghold */
  { W2_DEP_UPGRADE, 31, 0, W2_DEP_UPGRADE, 28 },  /* upgrade-berserker-regeneration <- upgrade-berserker */
  { W2_DEP_UPGRADE, 31, 1, W2_DEP_UNIT, 91 },  /* upgrade-berserker-regeneration <- unit-fortress */
  { W2_DEP_UPGRADE, 31, 1, W2_DEP_UPGRADE, 28 },  /* upgrade-berserker-regeneration <- upgrade-berserker */
  { W2_DEP_UPGRADE, 29, 0, W2_DEP_UNIT, 89 },  /* upgrade-light-axes <- unit-stronghold */
  { W2_DEP_UPGRADE, 29, 0, W2_DEP_UPGRADE, 28 },  /* upgrade-light-axes <- upgrade-berserker */
  { W2_DEP_UPGRADE, 29, 1, W2_DEP_UNIT, 91 },  /* upgrade-light-axes <- unit-fortress */
  { W2_DEP_UPGRADE, 29, 1, W2_DEP_UPGRADE, 28 },  /* upgrade-light-axes <- upgrade-berserker */
  { W2_DEP_UPGRADE, 44, 0, W2_DEP_UPGRADE, 32 },  /* upgrade-bloodlust <- upgrade-ogre-mage */
  { W2_DEP_UPGRADE, 50, 0, W2_DEP_UPGRADE, 32 },  /* upgrade-runes <- upgrade-ogre-mage */
};
const uint16_t w2_num_dependencies = 104;

const w2_spell w2_spells[W2_NUM_SPELLS] = {
  { "Holy Vision", 70, N, W2_ST_POSITION, W2_SE_REVEAL, W2UP_HOLY_VISION, N, 148, 0, 0, 25 },
  { "Healing", 6, 6, W2_ST_UNIT, W2_SE_HEAL, W2UP_HEALING, W2M_HEAL_EFFECT, 149, W2SF_ORGANIC_ONLY|W2SF_NO_BUILDING|W2SF_REPEAT, 1, 0 },
  { "Exorcism", 4, 10, W2_ST_UNIT, W2_SE_DAMAGE_UNDEAD, W2UP_EXORCISM, W2M_EXORCISM, 150, W2SF_UNDEAD_ONLY|W2SF_NO_BUILDING|W2SF_REPEAT, 1, 0 },
  { "Fireball", 100, 8, W2_ST_POSITION, W2_SE_MISSILE, W2UP_FIREBALL, W2M_FIREBALL, 144, 0, 20, 0 },
  { "Slow", 50, 10, W2_ST_UNIT, W2_SE_BUFF, W2UP_SLOW, N, 151, W2SF_NO_BUILDING, 0, 1000 },
  { "Flame Shield", 50, 6, W2_ST_UNIT, W2_SE_FLAME_SHIELD, W2UP_FLAME_SHIELD, W2M_FLAME_SHIELD, 152, W2SF_NO_BUILDING|W2SF_NO_AIR, 1, 600 },
  { "Invisibility", 200, 6, W2_ST_UNIT, W2_SE_BUFF, W2UP_INVISIBILITY, N, 153, W2SF_NO_BUILDING, 0, 2000 },
  { "Polymorph", 200, 10, W2_ST_UNIT, W2_SE_POLYMORPH, W2UP_POLYMORPH, N, 154, W2SF_ORGANIC_ONLY, 0, 0 },
  { "Blizzard", 25, 12, W2_ST_POSITION, W2_SE_AREA, W2UP_BLIZZARD, W2M_BLIZZARD, 155, W2SF_REPEAT, 11, 5 },
  { "Eye of Kilrogg", 70, 6, W2_ST_POSITION, W2_SE_EYE, W2UP_EYE_OF_KILROGG, N, 156, 0, 0, 765 },
  { "Bloodlust", 50, 6, W2_ST_UNIT, W2_SE_BUFF, W2UP_BLOODLUST, N, 157, W2SF_ORGANIC_ONLY, 0, 1000 },
  { "Runes", 200, 10, W2_ST_POSITION, W2_SE_RUNES, W2UP_RUNES, W2M_RUNE, 143, W2SF_NO_BUILDING, 50, 2000 },
  { "Death Coil", 100, 10, W2_ST_POSITION, W2_SE_MISSILE, W2UP_DEATH_COIL, W2M_DEATH_COIL, 158, W2SF_ORGANIC_ONLY, 50, 0 },
  { "Haste", 50, 6, W2_ST_UNIT, W2_SE_BUFF, W2UP_HASTE, N, 159, W2SF_NO_BUILDING, 0, 1000 },
  { "Raise Dead", 50, 6, W2_ST_POSITION, W2_SE_RAISE_DEAD, W2UP_RAISE_DEAD, N, N, W2SF_REPEAT, 0, 3600 },
  { "Whirlwind", 100, 12, W2_ST_POSITION, W2_SE_WHIRLWIND, W2UP_WHIRLWIND, W2M_WHIRLWIND, 160, 0, 3, 800 },
  { "Unholy Armor", 100, 6, W2_ST_UNIT, W2_SE_BUFF, W2UP_UNHOLY_ARMOR, N, 161, W2SF_NO_BUILDING|W2SF_HALVE_HP, 0, 500 },
  { "Death And Decay", 25, 12, W2_ST_POSITION, W2_SE_AREA, W2UP_DEATH_AND_DECAY, W2M_DEATH_AND_DECAY, 162, W2SF_REPEAT, 11, 5 },
  { "Demolish", 0, 1, W2_ST_SELF, W2_SE_DEMOLISH, N, W2M_EXPLOSION, 22, 0, 400, 3 },
};

const w2_construction w2_constructions[W2_NUM_CONSTRUCTIONS] = {
  { "land", {252, 262, 252, 252}, 64, 64, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "land2", {252, 262, 252, 252}, 64, 64, 2, { {0, 0, 0}, {25, 0, 1}, {0, 0, 0} } },
  { "wall", {261, 275, 276, 261}, 32, 32, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "human-shipyard", {253, 263, 253, 253}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "human-oil-well", {255, 265, 271, 271}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "human-refinery", {257, 267, 257, 257}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "human-foundry", {259, 269, 259, 259}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "orc-shipyard", {254, 264, 254, 254}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "orc-oil-well", {256, 266, 272, 272}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "orc-refinery", {258, 268, 258, 258}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
  { "orc-foundry", {260, 270, 260, 260}, 96, 96, 3, { {0, 0, 0}, {25, 0, 1}, {50, 1, 1} } },
};

const w2_button w2_buttons[] = {
  { W2U_FOOTMAN, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FOOTMAN, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FOOTMAN, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_FOOTMAN, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_FOOTMAN, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FOOTMAN, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_FOOTMAN, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_FOOTMAN, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FOOTMAN, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRUNT, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRUNT, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRUNT, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_GRUNT, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_GRUNT, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRUNT, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_GRUNT, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_GRUNT, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRUNT, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 4, 85, 'r', W2B_REPAIR, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 5, 86, 'h', W2B_HARVEST, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 6, 89, 'g', W2B_RETURN_GOODS, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 7, 87, 'b', W2B_BUTTON, 1, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 0, 8, 88, 'v', W2B_BUTTON, 2, W2R_UNITS_OR, {W2U_ELVEN_LUMBER_MILL, W2U_KEEP}, 0 },
  { W2U_PEASANT, 1, 1, 38, 'f', W2B_BUILD, W2U_FARM, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 2, 42, 'b', W2B_BUILD, W2U_HUMAN_BARRACKS, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 3, 40, 'h', W2B_BUILD, W2U_TOWN_HALL, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 4, 44, 'l', W2B_BUILD, W2U_ELVEN_LUMBER_MILL, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 5, 46, 's', W2B_BUILD, W2U_HUMAN_BLACKSMITH, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 7, 60, 't', W2B_BUILD, W2U_HUMAN_WATCH_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 1, 8, 92, 'w', W2B_BUILD, W2U_HUMAN_WALL, W2R_NETWORK, {N, N}, 0 },
  { W2U_PEASANT, 1, 8, 92, 'w', W2B_BUILD, W2U_HUMAN_WALL, W2R_DEBUG, {N, N}, 1 },
  { W2U_PEASANT, 1, 9, 91, 27, W2B_BUTTON, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 1, 48, 's', W2B_BUILD, W2U_HUMAN_SHIPYARD, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 2, 52, 'f', W2B_BUILD, W2U_HUMAN_FOUNDRY, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 3, 50, 'r', W2B_BUILD, W2U_HUMAN_REFINERY, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 4, 58, 'i', W2B_BUILD, W2U_INVENTOR, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 5, 56, 'a', W2B_BUILD, W2U_STABLES, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 6, 64, 'm', W2B_BUILD, W2U_MAGE_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 7, 62, 'c', W2B_BUILD, W2U_CHURCH, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 8, 72, 'g', W2B_BUILD, W2U_GRYPHON_AVIARY, W2R_NONE, {N, N}, 0 },
  { W2U_PEASANT, 2, 9, 91, 27, W2B_BUTTON, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 4, 85, 'r', W2B_REPAIR, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 5, 86, 'h', W2B_HARVEST, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 6, 90, 'g', W2B_RETURN_GOODS, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 7, 87, 'b', W2B_BUTTON, 1, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 0, 8, 88, 'v', W2B_BUTTON, 2, W2R_UNITS_OR, {W2U_TROLL_LUMBER_MILL, W2U_STRONGHOLD}, 0 },
  { W2U_PEON, 1, 1, 39, 'f', W2B_BUILD, W2U_PIG_FARM, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 2, 43, 'b', W2B_BUILD, W2U_ORC_BARRACKS, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 3, 41, 'h', W2B_BUILD, W2U_GREAT_HALL, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 4, 45, 'l', W2B_BUILD, W2U_TROLL_LUMBER_MILL, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 5, 47, 's', W2B_BUILD, W2U_ORC_BLACKSMITH, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 7, 61, 't', W2B_BUILD, W2U_ORC_WATCH_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 1, 8, 93, 'w', W2B_BUILD, W2U_ORC_WALL, W2R_NETWORK, {N, N}, 0 },
  { W2U_PEON, 1, 8, 93, 'w', W2B_BUILD, W2U_ORC_WALL, W2R_DEBUG, {N, N}, 1 },
  { W2U_PEON, 1, 9, 91, 27, W2B_BUTTON, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 1, 49, 's', W2B_BUILD, W2U_ORC_SHIPYARD, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 2, 53, 'f', W2B_BUILD, W2U_ORC_FOUNDRY, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 3, 51, 'r', W2B_BUILD, W2U_ORC_REFINERY, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 4, 59, 'a', W2B_BUILD, W2U_ALCHEMIST, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 5, 57, 'o', W2B_BUILD, W2U_OGRE_MOUND, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 6, 65, 't', W2B_BUILD, W2U_TEMPLE_OF_THE_DAMNED, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 7, 63, 'l', W2B_BUILD, W2U_ALTAR_OF_STORMS, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 8, 73, 'd', W2B_BUILD, W2U_DRAGON_ROOST, W2R_NONE, {N, N}, 0 },
  { W2U_PEON, 2, 9, 91, 27, W2B_BUTTON, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_BALLISTA, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_BALLISTA, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_BALLISTA, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_BALLISTA, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLISTA, 0, 6, 182, 'g', W2B_ATTACK_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_CATAPULT, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_CATAPULT, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_CATAPULT, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_CATAPULT, 0, 3, 138, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_CATAPULT1, N}, 0 },
  { W2U_CATAPULT, 0, 3, 139, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_CATAPULT2, N}, 0 },
  { W2U_CATAPULT, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_CATAPULT, 0, 6, 183, 'g', W2B_ATTACK_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_KNIGHT, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_KNIGHT, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_KNIGHT, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_KNIGHT, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_OGRE, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_OGRE, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_OGRE, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_OGRE, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_ARCHER, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_ARCHER, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_ARCHER, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_ARCHER, 0, 3, 124, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 3, 125, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW1, N}, 0 },
  { W2U_ARCHER, 0, 3, 126, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW2, N}, 0 },
  { W2U_ARCHER, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARCHER, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_AXETHROWER, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 127, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 128, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE1, N}, 0 },
  { W2U_AXETHROWER, 0, 3, 129, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE2, N}, 0 },
  { W2U_AXETHROWER, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_AXETHROWER, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAGE, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAGE, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAGE, 0, 3, 99, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAGE, 0, 4, 101, 'f', W2B_CAST_SPELL, W2SP_FIREBALL, W2R_UPGRADE, {W2UP_FIREBALL, N}, 0 },
  { W2U_MAGE, 0, 5, 94, 'o', W2B_CAST_SPELL, W2SP_SLOW, W2R_UPGRADE, {W2UP_SLOW, N}, 0 },
  { W2U_MAGE, 0, 6, 100, 'l', W2B_CAST_SPELL, W2SP_FLAME_SHIELD, W2R_UPGRADE, {W2UP_FLAME_SHIELD, N}, 0 },
  { W2U_MAGE, 0, 7, 95, 'i', W2B_CAST_SPELL, W2SP_INVISIBILITY, W2R_UPGRADE, {W2UP_INVISIBILITY, N}, 0 },
  { W2U_MAGE, 0, 8, 115, 'p', W2B_CAST_SPELL, W2SP_POLYMORPH, W2R_UPGRADE, {W2UP_POLYMORPH, N}, 0 },
  { W2U_MAGE, 0, 9, 105, 'b', W2B_CAST_SPELL, W2SP_BLIZZARD, W2R_UPGRADE, {W2UP_BLIZZARD, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 3, 102, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 4, 103, 'c', W2B_CAST_SPELL, W2SP_DEATH_COIL, W2R_UPGRADE, {W2UP_DEATH_COIL, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 5, 96, 'h', W2B_CAST_SPELL, W2SP_HASTE, W2R_UPGRADE, {W2UP_HASTE, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 6, 114, 'r', W2B_CAST_SPELL, W2SP_RAISE_DEAD, W2R_UPGRADE, {W2UP_RAISE_DEAD, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 7, 104, 'w', W2B_CAST_SPELL, W2SP_WHIRLWIND, W2R_UPGRADE, {W2UP_WHIRLWIND, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 8, 98, 'u', W2B_CAST_SPELL, W2SP_UNHOLY_ARMOR, W2R_UPGRADE, {W2UP_UNHOLY_ARMOR, N}, 0 },
  { W2U_DEATH_KNIGHT, 0, 9, 108, 'd', W2B_CAST_SPELL, W2SP_DEATH_AND_DECAY, W2R_UPGRADE, {W2UP_DEATH_AND_DECAY, N}, 0 },
  { W2U_PALADIN, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PALADIN, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PALADIN, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_PALADIN, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_PALADIN, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PALADIN, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_PALADIN, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_PALADIN, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PALADIN, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_PALADIN, 0, 7, 106, 'v', W2B_CAST_SPELL, W2SP_HOLY_VISION, W2R_UPGRADE, {W2UP_HOLY_VISION, N}, 0 },
  { W2U_PALADIN, 0, 8, 107, 'h', W2B_CAST_SPELL, W2SP_HEALING, W2R_UPGRADE, {W2UP_HEALING, N}, 0 },
  { W2U_PALADIN, 0, 9, 110, 'e', W2B_CAST_SPELL, W2SP_EXORCISM, W2R_UPGRADE, {W2UP_EXORCISM, N}, 0 },
  { W2U_OGRE_MAGE, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_MAGE, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_MAGE, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_OGRE_MAGE, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_OGRE_MAGE, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_MAGE, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_OGRE_MAGE, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_OGRE_MAGE, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_MAGE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_MAGE, 0, 7, 111, 'k', W2B_CAST_SPELL, W2SP_EYE_OF_VISION, W2R_UPGRADE, {W2UP_EYE_OF_KILROGG, N}, 0 },
  { W2U_OGRE_MAGE, 0, 8, 112, 'b', W2B_CAST_SPELL, W2SP_BLOODLUST, W2R_UPGRADE, {W2UP_BLOODLUST, N}, 0 },
  { W2U_OGRE_MAGE, 0, 9, 97, 'r', W2B_CAST_SPELL, W2SP_RUNES, W2R_UPGRADE, {W2UP_RUNES, N}, 0 },
  { W2U_DWARVES, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DWARVES, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DWARVES, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_DWARVES, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_DWARVES, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DWARVES, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_DWARVES, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_DWARVES, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DWARVES, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DWARVES, 0, 9, 142, 'd', W2B_DEMOLISH, 0, W2R_TRUE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GOBLIN_SAPPERS, 0, 9, 143, 'd', W2B_DEMOLISH, 0, W2R_TRUE, {N, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ATTACK_PEASANT, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_RANGER, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_RANGER, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_RANGER, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_RANGER, 0, 3, 124, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 3, 125, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW1, N}, 0 },
  { W2U_RANGER, 0, 3, 126, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW2, N}, 0 },
  { W2U_RANGER, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_RANGER, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_BERSERKER, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_BERSERKER, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_BERSERKER, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_BERSERKER, 0, 3, 127, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 3, 128, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE1, N}, 0 },
  { W2U_BERSERKER, 0, 3, 129, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE2, N}, 0 },
  { W2U_BERSERKER, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BERSERKER, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 3, 124, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 3, 125, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW1, N}, 0 },
  { W2U_FEMALE_HERO, 0, 3, 126, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ARROW2, N}, 0 },
  { W2U_FEMALE_HERO, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FEMALE_HERO, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 3, 102, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 4, 103, 'c', W2B_CAST_SPELL, W2SP_DEATH_COIL, W2R_UPGRADE, {W2UP_DEATH_COIL, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 5, 96, 'h', W2B_CAST_SPELL, W2SP_HASTE, W2R_UPGRADE, {W2UP_HASTE, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 6, 114, 'r', W2B_CAST_SPELL, W2SP_RAISE_DEAD, W2R_UPGRADE, {W2UP_RAISE_DEAD, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 7, 104, 'w', W2B_CAST_SPELL, W2SP_WHIRLWIND, W2R_UPGRADE, {W2UP_WHIRLWIND, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 8, 98, 'u', W2B_CAST_SPELL, W2SP_UNHOLY_ARMOR, W2R_UPGRADE, {W2UP_UNHOLY_ARMOR, N}, 0 },
  { W2U_EVIL_KNIGHT, 0, 9, 108, 'd', W2B_CAST_SPELL, W2SP_DEATH_AND_DECAY, W2R_UPGRADE, {W2UP_DEATH_AND_DECAY, N}, 0 },
  { W2U_FLYING_ANGEL, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FLYING_ANGEL, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FLYING_ANGEL, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FLYING_ANGEL, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FLYING_ANGEL, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_FAD_MAN, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_FAD_MAN, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_FAD_MAN, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_FAD_MAN, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FAD_MAN, 0, 7, 111, 'k', W2B_CAST_SPELL, W2SP_EYE_OF_VISION, W2R_UPGRADE, {W2UP_EYE_OF_KILROGG, N}, 0 },
  { W2U_FAD_MAN, 0, 8, 112, 'b', W2B_CAST_SPELL, W2SP_BLOODLUST, W2R_UPGRADE, {W2UP_BLOODLUST, N}, 0 },
  { W2U_FAD_MAN, 0, 9, 97, 'r', W2B_CAST_SPELL, W2SP_RUNES, W2R_UPGRADE, {W2UP_RUNES, N}, 0 },
  { W2U_WHITE_MAGE, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WHITE_MAGE, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WHITE_MAGE, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WHITE_MAGE, 0, 3, 99, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WHITE_MAGE, 0, 4, 101, 'f', W2B_CAST_SPELL, W2SP_FIREBALL, W2R_UPGRADE, {W2UP_FIREBALL, N}, 0 },
  { W2U_WHITE_MAGE, 0, 5, 94, 'o', W2B_CAST_SPELL, W2SP_SLOW, W2R_UPGRADE, {W2UP_SLOW, N}, 0 },
  { W2U_WHITE_MAGE, 0, 6, 100, 'l', W2B_CAST_SPELL, W2SP_FLAME_SHIELD, W2R_UPGRADE, {W2UP_FLAME_SHIELD, N}, 0 },
  { W2U_WHITE_MAGE, 0, 7, 95, 'i', W2B_CAST_SPELL, W2SP_INVISIBILITY, W2R_UPGRADE, {W2UP_INVISIBILITY, N}, 0 },
  { W2U_WHITE_MAGE, 0, 8, 115, 'p', W2B_CAST_SPELL, W2SP_POLYMORPH, W2R_UPGRADE, {W2UP_POLYMORPH, N}, 0 },
  { W2U_WHITE_MAGE, 0, 9, 105, 'b', W2B_CAST_SPELL, W2SP_BLIZZARD, W2R_UPGRADE, {W2UP_BLIZZARD, N}, 0 },
  { W2U_BEAST_CRY, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BEAST_CRY, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BEAST_CRY, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_BEAST_CRY, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_BEAST_CRY, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BEAST_CRY, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_BEAST_CRY, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_BEAST_CRY, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BEAST_CRY, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_OIL_TANKER, 0, 1, 157, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_OIL_TANKER, 0, 2, 153, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_OIL_TANKER, 0, 4, 54, 'b', W2B_BUILD, W2U_HUMAN_OIL_PLATFORM, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_OIL_TANKER, 0, 5, 161, 'h', W2B_HARVEST, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_OIL_TANKER, 0, 6, 159, 'g', W2B_RETURN_GOODS, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_OIL_TANKER, 0, 1, 156, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_OIL_TANKER, 0, 2, 150, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_OIL_TANKER, 0, 4, 55, 'b', W2B_BUILD, W2U_ORC_OIL_PLATFORM, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_OIL_TANKER, 0, 5, 160, 'h', W2B_HARVEST, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_OIL_TANKER, 0, 6, 158, 'g', W2B_RETURN_GOODS, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_TRANSPORT, 0, 1, 157, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_TRANSPORT, 0, 2, 153, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_TRANSPORT, 0, 3, 162, 'u', W2B_UNLOAD, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_TRANSPORT, 0, 1, 156, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_TRANSPORT, 0, 2, 150, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_TRANSPORT, 0, 3, 163, 'u', W2B_UNLOAD, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 1, 157, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 2, 153, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 3, 144, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 3, 145, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON1, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 3, 146, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON2, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 4, 184, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_DESTROYER, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 1, 156, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 2, 150, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 3, 147, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 3, 148, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON1, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 3, 149, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON2, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 4, 185, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_DESTROYER, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 1, 157, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 2, 153, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 3, 144, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 3, 145, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON1, N}, 0 },
  { W2U_BATTLESHIP, 0, 3, 146, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON2, N}, 0 },
  { W2U_BATTLESHIP, 0, 4, 184, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BATTLESHIP, 0, 6, 182, 'g', W2B_ATTACK_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 1, 156, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 2, 150, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 3, 147, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 3, 148, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON1, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 3, 149, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON2, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 4, 185, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_OGRE_JUGGERNAUGHT, 0, 6, 183, 'g', W2B_ATTACK_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FIRE_BREEZE, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FIRE_BREEZE, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FIRE_BREEZE, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FIRE_BREEZE, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_FIRE_BREEZE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 1, 157, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 2, 153, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 3, 144, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 3, 145, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON1, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 3, 146, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIP_CANNON2, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 4, 184, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SUBMARINE, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 1, 156, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 2, 150, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 3, 147, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 3, 148, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON1, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 3, 149, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_ORC_SHIP_CANNON2, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 4, 185, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SUBMARINE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLOON, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLOON, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_BALLOON, 0, 3, 184, 'x', W2B_EXPLORE, 0, W2R_NONE, {N, N}, 1 },
  { W2U_ZEPPELIN, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ZEPPELIN, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ZEPPELIN, 0, 3, 185, 'x', W2B_EXPLORE, 0, W2R_NONE, {N, N}, 1 },
  { W2U_GRYPHON_RIDER, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRYPHON_RIDER, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRYPHON_RIDER, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRYPHON_RIDER, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_GRYPHON_RIDER, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 7, 106, 'v', W2B_CAST_SPELL, W2SP_HOLY_VISION, W2R_UPGRADE, {W2UP_HOLY_VISION, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 8, 107, 'h', W2B_CAST_SPELL, W2SP_HEALING, W2R_UPGRADE, {W2UP_HEALING, N}, 0 },
  { W2U_KNIGHT_RIDER, 0, 9, 110, 'e', W2B_CAST_SPELL, W2SP_EXORCISM, W2R_UPGRADE, {W2UP_EXORCISM, N}, 0 },
  { W2U_EYE_OF_VISION, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EYE_OF_VISION, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_EYE_OF_VISION, 0, 3, 185, 'x', W2B_EXPLORE, 0, W2R_NONE, {N, N}, 1 },
  { W2U_ARTHOR_LITERIOS, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ARTHOR_LITERIOS, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_QUICK_BLADE, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_QUICK_BLADE, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_QUICK_BLADE, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_QUICK_BLADE, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_QUICK_BLADE, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_QUICK_BLADE, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_QUICK_BLADE, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_QUICK_BLADE, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_QUICK_BLADE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 2, 168, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD1, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 2, 169, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_ORC_SHIELD2, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 3, 120, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE1, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 3, 121, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_BATTLE_AXE2, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 7, 111, 'e', W2B_CAST_SPELL, W2SP_EYE_OF_VISION, W2R_TRUE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 8, 112, 'b', W2B_CAST_SPELL, W2SP_BLOODLUST, W2R_TRUE, {N, N}, 0 },
  { W2U_DOUBLE_HEAD, 0, 9, 97, 'r', W2B_CAST_SPELL, W2SP_RUNES, W2R_TRUE, {N, N}, 0 },
  { W2U_WISE_MAN, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WISE_MAN, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WISE_MAN, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_WISE_MAN, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_WISE_MAN, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WISE_MAN, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_WISE_MAN, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_WISE_MAN, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_WISE_MAN, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ICE_BRINGER, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ICE_BRINGER, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ICE_BRINGER, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ICE_BRINGER, 0, 3, 102, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_ICE_BRINGER, 0, 4, 103, 'c', W2B_CAST_SPELL, W2SP_DEATH_COIL, W2R_UPGRADE, {W2UP_DEATH_COIL, N}, 0 },
  { W2U_ICE_BRINGER, 0, 5, 96, 'h', W2B_CAST_SPELL, W2SP_HASTE, W2R_UPGRADE, {W2UP_HASTE, N}, 0 },
  { W2U_ICE_BRINGER, 0, 6, 114, 'r', W2B_CAST_SPELL, W2SP_RAISE_DEAD, W2R_UPGRADE, {W2UP_RAISE_DEAD, N}, 0 },
  { W2U_ICE_BRINGER, 0, 7, 104, 'w', W2B_CAST_SPELL, W2SP_WHIRLWIND, W2R_UPGRADE, {W2UP_WHIRLWIND, N}, 0 },
  { W2U_ICE_BRINGER, 0, 8, 98, 'u', W2B_CAST_SPELL, W2SP_UNHOLY_ARMOR, W2R_UPGRADE, {W2UP_UNHOLY_ARMOR, N}, 0 },
  { W2U_ICE_BRINGER, 0, 9, 108, 'd', W2B_CAST_SPELL, W2SP_DEATH_AND_DECAY, W2R_UPGRADE, {W2UP_DEATH_AND_DECAY, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 1, 83, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 2, 164, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 2, 165, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD1, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 2, 166, 's', W2B_STOP, 0, W2R_UPGRADE, {W2UP_HUMAN_SHIELD2, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 3, 116, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 3, 117, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD1, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 3, 118, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_SWORD2, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 4, 178, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 5, 180, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 7, 106, 'v', W2B_CAST_SPELL, W2SP_HOLY_VISION, W2R_UPGRADE, {W2UP_HOLY_VISION, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 8, 107, 'h', W2B_CAST_SPELL, W2SP_HEALING, W2R_UPGRADE, {W2UP_HEALING, N}, 0 },
  { W2U_MAN_OF_LIGHT, 0, 9, 110, 'e', W2B_CAST_SPELL, W2SP_EXORCISM, W2R_UPGRADE, {W2UP_EXORCISM, N}, 0 },
  { W2U_SHARP_AXE, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SHARP_AXE, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SHARP_AXE, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SHARP_AXE, 0, 3, 127, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SHARP_AXE, 0, 3, 128, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE1, N}, 0 },
  { W2U_SHARP_AXE, 0, 3, 129, 'a', W2B_ATTACK, 0, W2R_UPGRADE, {W2UP_THROWING_AXE2, N}, 0 },
  { W2U_SHARP_AXE, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SHARP_AXE, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SKELETON, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SKELETON, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SKELETON, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SKELETON, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_SKELETON, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DAEMON, 0, 1, 84, 'm', W2B_MOVE, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DAEMON, 0, 2, 167, 's', W2B_STOP, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DAEMON, 0, 3, 119, 'a', W2B_ATTACK, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DAEMON, 0, 4, 179, 'p', W2B_PATROL, 0, W2R_NONE, {N, N}, 0 },
  { W2U_DAEMON, 0, 5, 181, 't', W2B_STAND_GROUND, 0, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 1, 0, 'm', W2B_TRAIN_UNIT, W2U_ATTACK_PEASANT, W2R_NONE, {N, N}, 1 },
  { W2U_HUMAN_BARRACKS, 0, 1, 2, 'f', W2B_TRAIN_UNIT, W2U_FOOTMAN, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 2, 4, 'a', W2B_TRAIN_UNIT, W2U_ARCHER, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 2, 6, 'r', W2B_TRAIN_UNIT, W2U_RANGER, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 3, 16, 'b', W2B_TRAIN_UNIT, W2U_BALLISTA, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 4, 8, 'k', W2B_TRAIN_UNIT, W2U_KNIGHT, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_BARRACKS, 0, 4, 10, 'p', W2B_TRAIN_UNIT, W2U_PALADIN, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 1, 3, 'g', W2B_TRAIN_UNIT, W2U_GRUNT, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 2, 5, 'a', W2B_TRAIN_UNIT, W2U_AXETHROWER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 2, 7, 'b', W2B_TRAIN_UNIT, W2U_BERSERKER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 3, 17, 'c', W2B_TRAIN_UNIT, W2U_CATAPULT, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 4, 9, 'o', W2B_TRAIN_UNIT, W2U_OGRE, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_BARRACKS, 0, 4, 11, 'o', W2B_TRAIN_UNIT, W2U_OGRE_MAGE, W2R_NONE, {N, N}, 0 },
  { W2U_CHURCH, 0, 1, 10, 'p', W2B_RESEARCH, W2UP_PALADIN, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_CHURCH, 0, 2, 107, 'h', W2B_RESEARCH, W2UP_HEALING, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_CHURCH, 0, 3, 110, 'e', W2B_RESEARCH, W2UP_EXORCISM, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ALTAR_OF_STORMS, 0, 1, 11, 'm', W2B_RESEARCH, W2UP_OGRE_MAGE, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ALTAR_OF_STORMS, 0, 2, 112, 'b', W2B_RESEARCH, W2UP_BLOODLUST, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ALTAR_OF_STORMS, 0, 3, 97, 'r', W2B_RESEARCH, W2UP_RUNES, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_WATCH_TOWER, 0, 1, 75, 'g', W2B_UPGRADE_TO, W2U_HUMAN_GUARD_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_WATCH_TOWER, 0, 2, 76, 'c', W2B_UPGRADE_TO, W2U_HUMAN_CANNON_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_WATCH_TOWER, 0, 1, 77, 'g', W2B_UPGRADE_TO, W2U_ORC_GUARD_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_WATCH_TOWER, 0, 2, 78, 'c', W2B_UPGRADE_TO, W2U_ORC_CANNON_TOWER, W2R_NONE, {N, N}, 0 },
  { W2U_INVENTOR, 0, 1, 28, 'f', W2B_TRAIN_UNIT, W2U_BALLOON, W2R_NONE, {N, N}, 0 },
  { W2U_INVENTOR, 0, 2, 12, 'd', W2B_TRAIN_UNIT, W2U_DWARVES, W2R_NONE, {N, N}, 0 },
  { W2U_ALCHEMIST, 0, 1, 29, 'z', W2B_TRAIN_UNIT, W2U_ZEPPELIN, W2R_NONE, {N, N}, 0 },
  { W2U_ALCHEMIST, 0, 2, 13, 's', W2B_TRAIN_UNIT, W2U_GOBLIN_SAPPERS, W2R_NONE, {N, N}, 0 },
  { W2U_GRYPHON_AVIARY, 0, 1, 30, 'g', W2B_TRAIN_UNIT, W2U_GRYPHON_RIDER, W2R_NONE, {N, N}, 0 },
  { W2U_DRAGON_ROOST, 0, 1, 31, 'd', W2B_TRAIN_UNIT, W2U_DRAGON, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SHIPYARD, 0, 1, 18, 'o', W2B_TRAIN_UNIT, W2U_HUMAN_OIL_TANKER, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SHIPYARD, 0, 2, 22, 'd', W2B_TRAIN_UNIT, W2U_HUMAN_DESTROYER, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SHIPYARD, 0, 3, 20, 't', W2B_TRAIN_UNIT, W2U_HUMAN_TRANSPORT, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SHIPYARD, 0, 4, 26, 's', W2B_TRAIN_UNIT, W2U_HUMAN_SUBMARINE, W2R_NONE, {N, N}, 0 },
  { W2U_HUMAN_SHIPYARD, 0, 5, 24, 'b', W2B_TRAIN_UNIT, W2U_BATTLESHIP, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SHIPYARD, 0, 1, 19, 'o', W2B_TRAIN_UNIT, W2U_ORC_OIL_TANKER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SHIPYARD, 0, 2, 23, 'd', W2B_TRAIN_UNIT, W2U_ORC_DESTROYER, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SHIPYARD, 0, 3, 21, 't', W2B_TRAIN_UNIT, W2U_ORC_TRANSPORT, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SHIPYARD, 0, 4, 27, 'g', W2B_TRAIN_UNIT, W2U_ORC_SUBMARINE, W2R_NONE, {N, N}, 0 },
  { W2U_ORC_SHIPYARD, 0, 5, 25, 'j', W2B_TRAIN_UNIT, W2U_OGRE_JUGGERNAUGHT, W2R_NONE, {N, N}, 0 },
  { W2U_TOWN_HALL, 0, 1, 0, 'p', W2B_TRAIN_UNIT, W2U_PEASANT, W2R_NO_RESEARCH, {N, N}, 0 },
  { W2U_TOWN_HALL, 0, 2, 66, 'k', W2B_UPGRADE_TO, W2U_KEEP, W2R_UPGRADE_TO, {N, N}, 0 },
  { W2U_GREAT_HALL, 0, 1, 1, 'p', W2B_TRAIN_UNIT, W2U_PEON, W2R_NO_RESEARCH, {N, N}, 0 },
  { W2U_GREAT_HALL, 0, 2, 67, 's', W2B_UPGRADE_TO, W2U_STRONGHOLD, W2R_UPGRADE_TO, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 1, 125, 'u', W2B_RESEARCH, W2UP_ARROW1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 1, 126, 'u', W2B_RESEARCH, W2UP_ARROW2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 4, 6, 'r', W2B_RESEARCH, W2UP_RANGER, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 4, 133, 's', W2B_RESEARCH, W2UP_RANGER_SCOUTING, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 5, 132, 'l', W2B_RESEARCH, W2UP_LONGBOW, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ELVEN_LUMBER_MILL, 0, 6, 134, 'm', W2B_RESEARCH, W2UP_RANGER_MARKSMANSHIP, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 1, 128, 'u', W2B_RESEARCH, W2UP_THROWING_AXE1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 1, 129, 'u', W2B_RESEARCH, W2UP_THROWING_AXE2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 4, 7, 'b', W2B_RESEARCH, W2UP_BERSERKER, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 4, 136, 's', W2B_RESEARCH, W2UP_BERSERKER_SCOUTING, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 5, 135, 'a', W2B_RESEARCH, W2UP_LIGHT_AXES, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TROLL_LUMBER_MILL, 0, 6, 137, 'r', W2B_RESEARCH, W2UP_BERSERKER_REGENERATION, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_FOUNDRY, 0, 1, 145, 'c', W2B_RESEARCH, W2UP_HUMAN_SHIP_CANNON1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_FOUNDRY, 0, 1, 146, 'c', W2B_RESEARCH, W2UP_HUMAN_SHIP_CANNON2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_FOUNDRY, 0, 2, 154, 'a', W2B_RESEARCH, W2UP_HUMAN_SHIP_ARMOR1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_FOUNDRY, 0, 2, 155, 'a', W2B_RESEARCH, W2UP_HUMAN_SHIP_ARMOR2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_FOUNDRY, 0, 1, 148, 'c', W2B_RESEARCH, W2UP_ORC_SHIP_CANNON1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_FOUNDRY, 0, 1, 149, 'c', W2B_RESEARCH, W2UP_ORC_SHIP_CANNON2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_FOUNDRY, 0, 2, 151, 'a', W2B_RESEARCH, W2UP_ORC_SHIP_ARMOR1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_FOUNDRY, 0, 2, 152, 'a', W2B_RESEARCH, W2UP_ORC_SHIP_ARMOR2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 1, 14, 't', W2B_TRAIN_UNIT, W2U_MAGE, W2R_NONE, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 2, 94, 'o', W2B_RESEARCH, W2UP_SLOW, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 3, 100, 'l', W2B_RESEARCH, W2UP_FLAME_SHIELD, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 4, 95, 'i', W2B_RESEARCH, W2UP_INVISIBILITY, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 5, 115, 'p', W2B_RESEARCH, W2UP_POLYMORPH, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_MAGE_TOWER, 0, 6, 105, 'b', W2B_RESEARCH, W2UP_BLIZZARD, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 1, 15, 't', W2B_TRAIN_UNIT, W2U_DEATH_KNIGHT, W2R_NONE, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 2, 96, 'h', W2B_RESEARCH, W2UP_HASTE, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 3, 114, 'r', W2B_RESEARCH, W2UP_RAISE_DEAD, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 4, 104, 'w', W2B_RESEARCH, W2UP_WHIRLWIND, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 5, 98, 'u', W2B_RESEARCH, W2UP_UNHOLY_ARMOR, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_TEMPLE_OF_THE_DAMNED, 0, 6, 108, 'd', W2B_RESEARCH, W2UP_DEATH_AND_DECAY, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 1, 117, 'w', W2B_RESEARCH, W2UP_SWORD1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 1, 118, 'w', W2B_RESEARCH, W2UP_SWORD2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 2, 165, 's', W2B_RESEARCH, W2UP_HUMAN_SHIELD1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 2, 166, 's', W2B_RESEARCH, W2UP_HUMAN_SHIELD2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 3, 140, 'b', W2B_RESEARCH, W2UP_BALLISTA1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_HUMAN_BLACKSMITH, 0, 3, 141, 'b', W2B_RESEARCH, W2UP_BALLISTA2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 1, 120, 'w', W2B_RESEARCH, W2UP_BATTLE_AXE1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 1, 121, 'w', W2B_RESEARCH, W2UP_BATTLE_AXE2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 2, 168, 's', W2B_RESEARCH, W2UP_ORC_SHIELD1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 2, 169, 's', W2B_RESEARCH, W2UP_ORC_SHIELD2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 3, 138, 'c', W2B_RESEARCH, W2UP_CATAPULT1, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_ORC_BLACKSMITH, 0, 3, 139, 'c', W2B_RESEARCH, W2UP_CATAPULT2, W2R_SINGLE_RESEARCH, {N, N}, 0 },
  { W2U_KEEP, 0, 1, 0, 'p', W2B_TRAIN_UNIT, W2U_PEASANT, W2R_NO_RESEARCH, {N, N}, 0 },
  { W2U_KEEP, 0, 2, 68, 'c', W2B_UPGRADE_TO, W2U_CASTLE, W2R_UPGRADE_TO, {N, N}, 0 },
  { W2U_STRONGHOLD, 0, 1, 1, 'p', W2B_TRAIN_UNIT, W2U_PEON, W2R_NO_RESEARCH, {N, N}, 0 },
  { W2U_STRONGHOLD, 0, 2, 69, 'f', W2B_UPGRADE_TO, W2U_FORTRESS, W2R_UPGRADE_TO, {N, N}, 0 },
  { W2U_CASTLE, 0, 1, 0, 'p', W2B_TRAIN_UNIT, W2U_PEASANT, W2R_NO_RESEARCH, {N, N}, 0 },
  { W2U_FORTRESS, 0, 1, 1, 'p', W2B_TRAIN_UNIT, W2U_PEON, W2R_NO_RESEARCH, {N, N}, 0 },
  { N, 0, 9, 91, 27, W2B_CANCEL_UPGRADE, 0, W2R_NONE, {N, N}, 0 },
  { N, 0, 9, 91, 27, W2B_CANCEL_TRAIN_UNIT, 0, W2R_NONE, {N, N}, 0 },
  { N, 0, 9, 91, 27, W2B_CANCEL_BUILD, 0, W2R_NONE, {N, N}, 0 },
  { N, 9, 9, 91, 27, W2B_CANCEL, 0, W2R_NONE, {N, N}, 0 },
};
const uint16_t w2_num_buttons = 555;

/* per unit: first button index and count (0xFFFF = none) */
static const uint16_t w2_button_first[W2_NUM_UNIT_TYPES] = {
  0, 9, 18, 44, 70, 80, 92, 101, 110, 122, 134, 143,
  152, 164, 176, 186, 196, 0xFFFF, 205, 217, 229, 237, 247, 252,
  264, 274, 283, 288, 293, 296, 299, 306, 313, 321, 0xFFFF, 329,
  0xFFFF, 0xFFFF, 334, 341, 348, 351, 354, 359, 364, 376, 379, 388,
  0xFFFF, 397, 409, 418, 428, 440, 0xFFFF, 448, 453, 0xFFFF, 0xFFFF, 0xFFFF,
  458, 465, 471, 474, 477, 479, 0xFFFF, 0xFFFF, 481, 483, 485, 486,
  487, 492, 497, 499, 501, 507, 513, 517, 521, 527, 533, 539,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 545, 547, 549, 550, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
};
static const uint8_t w2_button_count[W2_NUM_UNIT_TYPES] = {
  9, 9, 26, 26, 10, 12, 9, 9, 12, 12, 9, 9, 12, 12, 10, 10,
  9, 0, 12, 12, 8, 10, 5, 12, 10, 9, 5, 5, 3, 3, 7, 7,
  8, 8, 0, 5, 0, 0, 7, 7, 3, 3, 5, 5, 12, 3, 9, 9,
  0, 12, 9, 10, 12, 8, 0, 5, 5, 0, 0, 0, 7, 6, 3, 3,
  2, 2, 0, 0, 2, 2, 1, 1, 5, 5, 2, 2, 6, 6, 4, 4,
  6, 6, 6, 6, 0, 0, 0, 0, 2, 2, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const w2_harvest w2_harvest_info[] = {
  { W2U_PEASANT, 1, 100, 0, 150, 150, 124, 0, 0, 0 },
  { W2U_PEASANT, 2, 100, 2, 24, 150, 122, 0, 1, 0 },
  { W2U_PEON, 1, 100, 0, 150, 150, 125, 0, 0, 0 },
  { W2U_PEON, 2, 100, 2, 24, 150, 123, 0, 1, 0 },
  { W2U_HUMAN_OIL_TANKER, 3, 100, 0, 100, 100, 126, 59, 0, 0 },
  { W2U_ORC_OIL_TANKER, 3, 100, 0, 100, 100, 127, 60, 0, 0 },
};
const uint8_t w2_num_harvest = 6;

const uint16_t w2_sound_entries[] = {
  5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 32, 34, 36,
  38, 43, 45, 49, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
  30, 33, 35, 37, 39, 44, 46, 50, 271, 272, 273, 274, 264, 265, 266, 267,
  268, 269, 270, 275, 276, 277, 278, 263, 115, 0xFFFF, 292, 31, 175, 176, 177, 178,
  171, 172, 173, 179, 180, 181, 182, 174, 201, 202, 203, 204, 195, 196, 197, 198,
  199, 205, 206, 207, 200, 140, 141, 142, 143, 136, 137, 138, 144, 145, 146, 147,
  139, 247, 248, 249, 243, 244, 245, 250, 251, 252, 246, 257, 258, 259, 253, 254,
  255, 260, 261, 262, 256, 120, 121, 116, 117, 118, 122, 123, 124, 119, 187, 188,
  189, 190, 183, 184, 185, 191, 192, 193, 194, 186, 212, 213, 214, 215, 208, 209,
  210, 216, 217, 218, 211, 129, 130, 125, 126, 127, 131, 132, 133, 134, 135, 128,
  159, 160, 161, 162, 155, 156, 157, 163, 164, 165, 166, 158, 285, 227, 229, 231,
  219, 221, 223, 78, 225, 51, 228, 230, 232, 220, 222, 224, 226, 233, 235, 237,
  234, 236, 238, 281, 282, 239, 240, 241, 242, 148, 149, 150, 151, 152, 154, 153,
  167, 168, 170, 169, 283, 284, 280, 279, 74, 47, 52, 53, 54, 75, 48, 70,
  71, 72, 73, 90, 91, 87, 88, 80, 84, 89, 92, 93, 69, 83, 82, 76,
  60, 61, 62, 81, 56, 57, 58, 59, 55, 63, 66, 77, 111, 112, 65, 79,
  64, 67, 105, 104, 101, 110, 102, 107, 109, 106, 108, 98, 100, 103, 114, 113,
  99, 2, 3, 4, 42, 41, 40, 41, 86,
};
const w2_sound w2_sounds[] = {
  { "basic human voices selected", 0, 6, N },  /* 0 */
  { "basic human voices annoyed", 6, 7, N },  /* 1 */
  { "footman-selected", 0, 6, 1 },  /* 2 */
  { "basic human voices acknowledge", 13, 4, N },  /* 3 */
  { "basic human voices ready", 17, 1, N },  /* 4 */
  { "basic human voices help 1", 18, 1, N },  /* 5 */
  { "basic human voices dead", 19, 1, N },  /* 6 */
  { "basic orc voices selected", 20, 6, N },  /* 7 */
  { "basic orc voices annoyed", 26, 7, N },  /* 8 */
  { "grunt-selected", 20, 6, 8 },  /* 9 */
  { "basic orc voices acknowledge", 33, 4, N },  /* 10 */
  { "basic orc voices ready", 37, 1, N },  /* 11 */
  { "basic orc voices help 1", 38, 1, N },  /* 12 */
  { "basic orc voices dead", 39, 1, N },  /* 13 */
  { "peasant selected", 40, 4, N },  /* 14 */
  { "peasant annoyed", 44, 7, N },  /* 15 */
  { "peasant-selected", 40, 4, 15 },  /* 16 */
  { "peasant-acknowledge", 51, 4, N },  /* 17 */
  { "peasant-ready", 55, 1, N },  /* 18 */
  { "peon-ready", 56, 1, N },  /* 19 */
  { "click", 57, 1, N },  /* 20 */
  { "catapult-ballista movement", 58, 1, N },  /* 21 */
  { "explosion", 59, 1, N },  /* 22 */
  { "knight selected", 60, 4, N },  /* 23 */
  { "knight annoyed", 64, 3, N },  /* 24 */
  { "knight-selected", 60, 4, 24 },  /* 25 */
  { "knight-acknowledge", 67, 4, N },  /* 26 */
  { "knight-ready", 71, 1, N },  /* 27 */
  { "ogre selected", 72, 4, N },  /* 28 */
  { "ogre annoyed", 76, 5, N },  /* 29 */
  { "ogre-selected", 72, 4, 29 },  /* 30 */
  { "ogre-acknowledge", 81, 3, N },  /* 31 */
  { "ogre-ready", 84, 1, N },  /* 32 */
  { "elven archer-ranger selected", 85, 4, N },  /* 33 */
  { "elven archer-ranger annoyed", 89, 3, N },  /* 34 */
  { "archer-selected", 85, 4, 34 },  /* 35 */
  { "elven archer-ranger acknowledge", 92, 4, N },  /* 36 */
  { "elven archer-ranger ready", 96, 1, N },  /* 37 */
  { "troll axethrower-berserker selected", 97, 3, N },  /* 38 */
  { "troll axethrower-berserker annoyed", 100, 3, N },  /* 39 */
  { "axethrower-selected", 97, 3, 39 },  /* 40 */
  { "troll axethrower-berserker acknowledge", 103, 3, N },  /* 41 */
  { "troll axethrower-berserker ready", 106, 1, N },  /* 42 */
  { "mage selected", 107, 3, N },  /* 43 */
  { "mage annoyed", 110, 3, N },  /* 44 */
  { "mage-selected", 107, 3, 44 },  /* 45 */
  { "mage-acknowledge", 113, 3, N },  /* 46 */
  { "mage-ready", 116, 1, N },  /* 47 */
  { "death knight selected", 117, 2, N },  /* 48 */
  { "death knight annoyed", 119, 3, N },  /* 49 */
  { "death-knight-selected", 117, 2, 49 },  /* 50 */
  { "death-knight-acknowledge", 122, 3, N },  /* 51 */
  { "death-knight-ready", 125, 1, N },  /* 52 */
  { "paladin selected", 126, 4, N },  /* 53 */
  { "paladin annoyed", 130, 3, N },  /* 54 */
  { "paladin-selected", 126, 4, 54 },  /* 55 */
  { "paladin-acknowledge", 133, 4, N },  /* 56 */
  { "paladin-ready", 137, 1, N },  /* 57 */
  { "ogre-mage selected", 138, 4, N },  /* 58 */
  { "ogre-mage annoyed", 142, 3, N },  /* 59 */
  { "ogre-mage-selected", 138, 4, 59 },  /* 60 */
  { "ogre-mage-acknowledge", 145, 3, N },  /* 61 */
  { "ogre-mage-ready", 148, 1, N },  /* 62 */
  { "dwarven demolition squad selected", 149, 2, N },  /* 63 */
  { "dwarven demolition squad annoyed", 151, 3, N },  /* 64 */
  { "dwarves-selected", 149, 2, 64 },  /* 65 */
  { "dwarves-acknowledge", 154, 5, N },  /* 66 */
  { "dwarves-ready", 159, 1, N },  /* 67 */
  { "goblin sappers selected", 160, 4, N },  /* 68 */
  { "goblin sappers annoyed", 164, 3, N },  /* 69 */
  { "goblin-sappers-selected", 160, 4, 69 },  /* 70 */
  { "goblin-sappers-acknowledge", 167, 4, N },  /* 71 */
  { "goblin-sappers-ready", 171, 1, N },  /* 72 */
  { "ranger-selected", 85, 4, 34 },  /* 73 */
  { "berserker-selected", 97, 3, 39 },  /* 74 */
  { "alleria-selected", 85, 4, 34 },  /* 75 */
  { "teron-gorefiend-selected", 20, 6, 8 },  /* 76 */
  { "kurdan-and-sky-ree-selected", 0, 6, 1 },  /* 77 */
  { "gryphon-rider-acknowledge", 172, 1, N },  /* 78 */
  { "dentarg-selected", 72, 4, 29 },  /* 79 */
  { "khadgar-selected", 107, 3, 44 },  /* 80 */
  { "grom-hellscream-selected", 20, 6, 8 },  /* 81 */
  { "ships human selected", 173, 3, N },  /* 82 */
  { "ships human annoyed", 176, 3, N },  /* 83 */
  { "human-oil-tanker-selected", 173, 3, 83 },  /* 84 */
  { "tanker acknowledge", 179, 1, N },  /* 85 */
  { "ships human ready", 180, 1, N },  /* 86 */
  { "ship sinking", 181, 1, N },  /* 87 */
  { "ships orc selected", 182, 3, N },  /* 88 */
  { "ships orc annoyed", 185, 3, N },  /* 89 */
  { "orc-oil-tanker-selected", 182, 3, 89 },  /* 90 */
  { "ships orc ready", 188, 1, N },  /* 91 */
  { "ships human acknowledge", 189, 3, N },  /* 92 */
  { "ships orc acknowledge", 192, 3, N },  /* 93 */
  { "deathwing-selected", 20, 6, 8 },  /* 94 */
  { "dragon-acknowledge", 195, 2, N },  /* 95 */
  { "ships submarine annoyed", 197, 4, N },  /* 96 */
  { "gnomish-submarine-selected", 173, 3, 96 },  /* 97 */
  { "gnomish flying machine annoyed", 201, 5, N },  /* 98 */
  { "gnomish-flying-machine-selected", 57, 1, 98 },  /* 99 */
  { "gnomish-flying-machine-acknowledge", 206, 1, N },  /* 100 */
  { "gnomish-flying-machine-ready", 207, 1, N },  /* 101 */
  { "goblin zeppelin annoyed", 208, 2, N },  /* 102 */
  { "goblin-zeppelin-selected", 57, 1, 102 },  /* 103 */
  { "goblin-zeppelin-acknowledge", 210, 1, N },  /* 104 */
  { "goblin-zeppelin-ready", 211, 1, N },  /* 105 */
  { "gryphon-rider-selected", 212, 1, N },  /* 106 */
  { "gryphon-rider-ready", 213, 1, N },  /* 107 */
  { "dragon-selected", 214, 1, N },  /* 108 */
  { "dragon-ready", 215, 1, N },  /* 109 */
  { "turalyon-selected", 60, 4, 24 },  /* 110 */
  { "danath-selected", 0, 6, 1 },  /* 111 */
  { "korgath-bladefist-selected", 20, 6, 8 },  /* 112 */
  { "farm-selected", 216, 1, N },  /* 113 */
  { "basic human voices help 2", 217, 1, N },  /* 114 */
  { "building destroyed", 218, 3, N },  /* 115 */
  { "pig-farm-selected", 221, 1, N },  /* 116 */
  { "basic orc voices help 2", 222, 1, N },  /* 117 */
  { "church-selected", 223, 1, N },  /* 118 */
  { "altar-of-storms-selected", 224, 1, N },  /* 119 */
  { "stables-selected", 225, 1, N },  /* 120 */
  { "ogre-mound-selected", 226, 1, N },  /* 121 */
  { "gnomish-inventor-selected", 227, 1, N },  /* 122 */
  { "goblin-alchemist-selected", 228, 1, N },  /* 123 */
  { "gryphon-aviary-selected", 229, 1, N },  /* 124 */
  { "dragon-roost-selected", 230, 1, N },  /* 125 */
  { "shipyard", 231, 1, N },  /* 126 */
  { "lumbermill", 232, 1, N },  /* 127 */
  { "foundry", 233, 1, N },  /* 128 */
  { "mage-tower-selected", 234, 1, N },  /* 129 */
  { "temple-of-the-damned-selected", 235, 1, N },  /* 130 */
  { "blacksmith", 236, 1, N },  /* 131 */
  { "oil refinery", 237, 1, N },  /* 132 */
  { "oil platform", 238, 1, N },  /* 133 */
  { "gold-mine-selected", 239, 1, N },  /* 134 */
  { "sword attack", 240, 3, N },  /* 135 */
  { "peasant attack", 243, 1, N },  /* 136 */
  { "tree-chopping", 244, 4, N },  /* 137 */
  { "catapult-ballista attack", 248, 1, N },  /* 138 */
  { "punch", 249, 1, N },  /* 139 */
  { "bow throw", 250, 1, N },  /* 140 */
  { "axe throw", 251, 1, N },  /* 141 */
  { "lightning", 252, 1, N },  /* 142 */
  { "touch of darkness", 253, 1, N },  /* 143 */
  { "fireball throw", 254, 1, N },  /* 144 */
  { "fist", 255, 1, N },  /* 145 */
  { "fireball hit", 256, 1, N },  /* 146 */
  { "bow hit", 257, 1, N },  /* 147 */
  { "holy vision", 258, 1, N },  /* 148 */
  { "healing", 259, 1, N },  /* 149 */
  { "exorcism", 260, 1, N },  /* 150 */
  { "slow", 261, 1, N },  /* 151 */
  { "flame shield", 262, 1, N },  /* 152 */
  { "invisibility", 263, 1, N },  /* 153 */
  { "polymorph", 264, 1, N },  /* 154 */
  { "blizzard", 265, 1, N },  /* 155 */
  { "eye of vision", 266, 1, N },  /* 156 */
  { "bloodlust", 267, 1, N },  /* 157 */
  { "death coil", 268, 1, N },  /* 158 */
  { "haste", 269, 1, N },  /* 159 */
  { "whirlwind", 270, 1, N },  /* 160 */
  { "unholy armor", 271, 1, N },  /* 161 */
  { "death and decay", 272, 1, N },  /* 162 */
  { "placement error", 273, 1, N },  /* 163 */
  { "placement success", 274, 1, N },  /* 164 */
  { "building construction", 275, 1, N },  /* 165 */
  { "peasant voice work complete", 276, 1, N },  /* 166 */
  { "basic orc voices work complete", 277, 1, N },  /* 167 */
  { "basic human voices research complete", 278, 1, N },  /* 168 */
  { "basic orc voices research complete", 279, 1, N },  /* 169 */
  { "burning", 280, 1, N },  /* 170 */
};
const uint8_t w2_num_sounds = 171;

const w2_era_info w2_eras[W2_NUM_ERAS] = {
  { "forest",    2,  {2, 3, 4, 5},     356 },
  { "winter",    18, {18, 19, 20, 21}, 357 },
  { "wasteland", 10, {10, 11, 12, 13}, 358 },
  { "swamp",     10, {10, 11, 12, 13}, 358 },  /* expansion only; Mac 1.2x has no swamp data -> wasteland */
};

/* NIEPEWNE: original menu has a speed slider; only "normal" = 30 c/s is confirmed (Wargus default).
   Wargus allows 15..75 in steps of 5. */
const w2_game_speed w2_game_speeds[] = {
  { "slowest", 15 }, { "slow", 22 }, { "normal", 30 }, { "fast", 45 }, { "fastest", 60 },
};
const uint8_t w2_num_game_speeds = 5;

const w2_unit_type *w2_unit(unsigned id)
{
    return id < W2_NUM_UNIT_TYPES ? &w2_unit_types[id] : NULL;
}

const w2_anim_step *w2_anim(unsigned set, unsigned kind, unsigned *count)
{
    const w2_anim_set *s;
    if (set >= W2_NUM_ANIM_SETS || kind >= W2_NUM_ANIM_KINDS) { if (count) *count = 0; return NULL; }
    s = &w2_anim_sets[set];
    if (s->first[kind] == 0xFFFFu) { if (count) *count = 0; return NULL; }
    if (count) *count = s->count[kind];
    return &w2_anim_steps[s->first[kind]];
}

const w2_anim_step *w2_unit_anim(unsigned unit_id, unsigned kind, unsigned *count)
{
    const w2_unit_type *u = w2_unit(unit_id);
    if (!u || u->anim == W2_NONE) { if (count) *count = 0; return NULL; }
    return w2_anim(u->anim, kind, count);
}

const w2_button *w2_unit_buttons(unsigned unit_id, unsigned *count)
{
    if (unit_id >= W2_NUM_UNIT_TYPES || w2_button_first[unit_id] == 0xFFFFu) { if (count) *count = 0; return NULL; }
    if (count) *count = w2_button_count[unit_id];
    return &w2_buttons[w2_button_first[unit_id]];
}

const w2_harvest *w2_unit_harvest(unsigned unit_id, unsigned resource)
{
    unsigned i;
    for (i = 0; i < w2_num_harvest; ++i)
        if (w2_harvest_info[i].unit == unit_id && w2_harvest_info[i].resource == resource)
            return &w2_harvest_info[i];
    return NULL;
}

unsigned w2_dir_frame(unsigned base, unsigned heading, int *mirror)
{
    heading &= 7u;
    if (heading <= 4u) { if (mirror) *mirror = 0; return base + heading; }
    if (mirror) *mirror = 1;
    return base + (8u - heading);   /* 5=SW->SE(3), 6=W->E(2), 7=NW->NE(1) */
}

unsigned w2_time_to_cycles(unsigned time_units)
{
    return time_units * W2_TIME_UNIT_CYCLES;
}
