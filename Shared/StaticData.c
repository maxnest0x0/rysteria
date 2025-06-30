// Copyright (C) 2024 Paul Johnson
// Copyright (C) 2024-2025 Maxim Nesterov

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <Shared/StaticData.h>

#include <math.h>
#include <stdio.h>

#include <Shared/Utilities.h>

// clang-format off
struct rr_petal_base_stat_scale const offensive[rr_rarity_id_max] = {
    {1.0, 1.0},
    {1.7, 2.0},
    {2.9, 4.0},
    {5.0, 8.0},
    {8.5,  16},
    {14.5, 48},
    {24.6,144},
    {42.0,432}
};

struct rr_petal_base_stat_scale const defensive[rr_rarity_id_max] = {
    {1.0, 1.0},
    {2.0, 1.7},
    {4.0, 2.9},
    {8.0, 5.0},
    {16,  8.5},
    {48, 14.5},
    {144,24.6},
    {432,42.0}
};

struct rr_petal_data RR_PETAL_DATA[rr_petal_id_max] = {
//   id                     min_rarity              scale        dmg      hp   clump   cd 2cd  count
    {rr_petal_id_none,      rr_rarity_id_common,    offensive,  0.0f,   0.0f,   0.0f,   0,  0, {0,0,0,0,0,0,0,0}},
    {rr_petal_id_basic,     rr_rarity_id_common,    offensive, 10.0f,  15.0f,   0.0f,  50,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_pellet,    rr_rarity_id_common,    offensive,  8.0f,   5.0f,   0.0f,  13,  0, {1,2,2,3,3,5,6,6}},
    {rr_petal_id_fossil,    rr_rarity_id_common,    offensive,  5.0f, 100.0f,   0.0f, 100,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_stinger,   rr_rarity_id_common,    offensive, 65.0f,   3.0f,  10.0f, 150,  0, {1,1,1,1,1,3,5,5}},
    {rr_petal_id_berry,     rr_rarity_id_rare,      offensive,  5.0f,   5.0f,  12.0f,  13,  0, {1,1,1,1,1,2,3,3}},
    {rr_petal_id_shell,     rr_rarity_id_rare,      offensive, 30.0f,  16.0f,   0.0f,  50, 13, {1,1,1,1,1,2,3,3}},
    {rr_petal_id_peas,      rr_rarity_id_rare,      offensive, 22.0f,  12.0f,   8.0f,  13, 12, {4,4,4,4,4,4,5,5}},
    {rr_petal_id_leaf,      rr_rarity_id_unusual,   offensive,  9.0f,   8.0f,   8.0f,  38,  0, {1,1,1,1,1,2,2,2}},
    {rr_petal_id_egg,       rr_rarity_id_unusual,   defensive,  1.0f,  75.0f,  10.0f,  25,100, {3,3,3,2,2,2,1,1}},
    {rr_petal_id_magnet,    rr_rarity_id_rare,      defensive,  2.0f,  25.0f,   0.0f,  38,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_uranium,   rr_rarity_id_rare,      offensive,  3.0f,  40.0f,   0.0f,  25, 25, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_feather,   rr_rarity_id_common,    defensive,  1.0f,   3.0f,   0.0f,  25,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_azalea,    rr_rarity_id_common,    defensive,  5.0f,  15.0f,  10.0f,  25, 50, {1,1,1,1,1,1,3,3}},
    {rr_petal_id_bone,      rr_rarity_id_common,    defensive,  2.5f,  25.0f,   0.0f,  68,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_web,       rr_rarity_id_rare,      defensive,  5.0f,   5.0f,   0.0f,  50, 13, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_seed,      rr_rarity_id_legendary, defensive,  1.0f,  75.0f,   0.0f,  63,  1, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_gravel,    rr_rarity_id_unusual,   offensive,  9.0f,  20.0f,   0.0f,  20, 10, {1,2,2,2,3,3,4,4}},
    {rr_petal_id_club,      rr_rarity_id_common,    defensive,  8.0f, 600.0f,   0.0f, 250,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_crest,     rr_rarity_id_rare,      offensive,  0.0f,   0.0f,   0.0f,   0,  0, {0,0,0,0,0,0,0,0}},
    {rr_petal_id_droplet,   rr_rarity_id_common,    offensive, 15.0f,   5.0f,   0.0f,  37,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_beak,      rr_rarity_id_unusual,   defensive,  5.0f,   5.0f,   0.0f,  55,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_lightning, rr_rarity_id_unusual,   offensive,  7.0f,   1.0f,   0.0f,  63,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_third_eye, rr_rarity_id_legendary, offensive,  0.0f,   0.0f,   0.0f,   0,  0, {0,0,0,0,0,0,0,0}},
    {rr_petal_id_nest,      rr_rarity_id_legendary, defensive,  5.0f,  25.0f,   0.0f, 125,  1, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_fireball,  rr_rarity_id_unusual,   offensive,260.0f,   1.0f,   0.0f, 600,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_meat,      rr_rarity_id_common,    offensive,  0.0f,1600.0f,   0.0f, 188, 13, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_bubble,    rr_rarity_id_common,    defensive,  1.0f,  25.0f,   0.0f, 200,  3, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_meteor,    rr_rarity_id_unusual,   defensive,  0.0f,   0.0f,   0.0f,  50,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_mandible,  rr_rarity_id_common,    offensive,  5.0f,  10.0f,   0.0f,  75,  0, {1,1,1,1,1,1,1,1}},
    {rr_petal_id_wax,       rr_rarity_id_unusual,   offensive, 10.0f,  10.0f,  10.0f,  38,  0, {2,2,2,2,2,2,2,2}},
    {rr_petal_id_sand,      rr_rarity_id_common,    offensive, 15.0f,  10.0f,  10.0f,  37,  0, {4,4,4,4,4,4,4,4}},
    {rr_petal_id_mint,      rr_rarity_id_unusual,   offensive,  5.0f,  10.0f,  10.0f,  50, 25, {1,1,1,1,1,1,1,1}},
};    

char const *RR_PETAL_NAMES[rr_petal_id_max] = {
    "Secret",   "Petal",     "Pellet",    "Fossil", "Stinger",  "Berry",   "Shell",
    "Peas",     "Leaf",      "Egg",       "Magnet", "Uranium",  "Feather", "Azalea",
    "Bone",     "Web",       "Seed",      "Gravel", "Club",     "Crest",   "Droplet",
    "Beak",     "Lightning", "Third Eye", "Nest",   "Fireball", "Meat",    "Bubble",
    "Meteor",   "Mandible",  "Wax",       "Sand",   "Mint",
};
    
char const *RR_PETAL_DESCRIPTIONS[rr_petal_id_max] = {
    0,
    "It's just a petal",
    "Low damage, but there's lots",
    "It came from a dino",
    "Ow that hurts",
    "Gives your other petals more energy",
    "Poor snail",
    "Splits in 4. Or maybe 5 if you're a pro",
    "Heals you gradually",
    "Spawns a pet dinosaur to protect you",
    "Increases loot pickup radius. Stacks diminishingly",
    "Does damage to the owner and enemies in a large range",
    "It's so light it increases your movement speed. Stacks diminishingly",
    "It heals you",
    "Gives the player armor. Stacks diminishingly",
    "It slows everything down",
    "What does this one do",
    "Tiny rocks that stay on the ground and trip dinos",
    "Heavy and sturdy",
    "Increases your maximum vision range. Does not stack",
    "This mysterious petal reverses your petal rotation",
    "Stuns mobs and prevents them from moving",
    "A stunning display",
    "Your petals hate it and want to move further away. Stacks diminishingly",
    "Home sweet home",
    "Nice ball bro",
    "Meat meta",
    "Pop and you're gone",
    "Spawns a pet meteor to protect you",
    "Does more damage if target hp is below 50%",
    "Made by the bees",
    "Very fine",
    "Remember to feed your pets"
};

struct rr_mob_data RR_MOB_DATA[rr_mob_id_max] = {
//   id                                     min_rarity  max_rarity              hp dmg    rad  ai_passive_rarity    ai_neutral_rarity    ai_aggro_rarity       loot
    {rr_mob_id_triceratops,        rr_rarity_id_common, rr_rarity_id_ultimate,  45, 15, 30.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_max,     {{rr_petal_id_leaf,    0.15},{rr_petal_id_fossil,    0.05}}},
    {rr_mob_id_trex,               rr_rarity_id_common, rr_rarity_id_ultimate,  40, 25, 32.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_unusual, {{rr_petal_id_stinger, 0.03},{rr_petal_id_egg,       0.05},{rr_petal_id_meat,      0.01}}},
    {rr_mob_id_fern,               rr_rarity_id_common, rr_rarity_id_ultimate,  10,  5, 24.0f, rr_rarity_id_max,    rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_leaf,     0.1},{rr_petal_id_azalea,    0.25}}},
    {rr_mob_id_tree,               rr_rarity_id_common, rr_rarity_id_ultimate, 100,  5, 64.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_common,  {{rr_petal_id_leaf,     2.5},{rr_petal_id_peas,       2.5},{rr_petal_id_seed,      0.05}}}, // :trol:
    {rr_mob_id_pteranodon,         rr_rarity_id_common, rr_rarity_id_ultimate,  40, 15, 20.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_rare,    {{rr_petal_id_shell,   0.05},{rr_petal_id_beak,      0.15},{rr_petal_id_nest,      0.01}}},
    {rr_mob_id_dakotaraptor,       rr_rarity_id_common, rr_rarity_id_ultimate,  35, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_epic,    {{rr_petal_id_crest,    0.1},{rr_petal_id_feather,    0.1},{rr_petal_id_pellet,    0.05}}},
    {rr_mob_id_pachycephalosaurus, rr_rarity_id_common, rr_rarity_id_ultimate,  35, 20, 20.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_common,  {{rr_petal_id_fossil,   0.1},{rr_petal_id_berry,      0.1},{rr_petal_id_web,       0.05}}},
    {rr_mob_id_ornithomimus,       rr_rarity_id_common, rr_rarity_id_ultimate,  25, 10, 20.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_max,     {{rr_petal_id_feather,  0.1},{rr_petal_id_droplet,   0.05},{rr_petal_id_pellet,     0.1}}},
    {rr_mob_id_ankylosaurus,       rr_rarity_id_common, rr_rarity_id_ultimate,  50, 10, 30.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_max,     {{rr_petal_id_club,    0.15},{rr_petal_id_gravel,    0.05},{rr_petal_id_bubble,     0.1}}},
    {rr_mob_id_meteor,             rr_rarity_id_common, rr_rarity_id_ultimate, 100, 10, 32.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_magnet,   0.5},{rr_petal_id_uranium,   0.05},{rr_petal_id_fireball,   1.0},{rr_petal_id_meteor, 2.0}}},
    {rr_mob_id_quetzalcoatlus,     rr_rarity_id_common, rr_rarity_id_ultimate,  65, 20, 28.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_common,  {{rr_petal_id_beak,    0.05},{rr_petal_id_fossil,     0.1},{rr_petal_id_lightning, 0.01}}},
    {rr_mob_id_edmontosaurus,      rr_rarity_id_common, rr_rarity_id_ultimate,  50, 15, 30.0f, rr_rarity_id_common, rr_rarity_id_common, rr_rarity_id_max,     {{rr_petal_id_bone,    0.01},{rr_petal_id_fossil,     0.1},{rr_petal_id_third_eye, 0.05}}},
    {rr_mob_id_ant,                rr_rarity_id_common, rr_rarity_id_ultimate,  10, 10, 20.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_pellet,   0.1},{rr_petal_id_leaf,       0.1},{rr_petal_id_mandible,  0.05}}},
    {rr_mob_id_hornet,             rr_rarity_id_common, rr_rarity_id_ultimate,  28, 25, 25.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_stinger,  0.1},{rr_petal_id_crest,     0.05}}},
    {rr_mob_id_dragonfly,          rr_rarity_id_common, rr_rarity_id_ultimate,  20, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_pellet,   0.1},{rr_petal_id_magnet,    0.05}}},
    {rr_mob_id_honeybee,           rr_rarity_id_common, rr_rarity_id_ultimate,  12, 25, 22.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_wax,     0.05},{rr_petal_id_stinger,   0.05}}},
    {rr_mob_id_beehive,            rr_rarity_id_common, rr_rarity_id_ultimate,   0,  0, 45.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_wax,     0.05},{rr_petal_id_azalea,    0.05}}},
    {rr_mob_id_spider,             rr_rarity_id_common, rr_rarity_id_ultimate,  20, 25, 25.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_web,      0.1},{rr_petal_id_third_eye, 0.01}}},
    {rr_mob_id_house_centipede,    rr_rarity_id_common, rr_rarity_id_ultimate,  25, 10, 23.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_peas,     0.1},{rr_petal_id_sand,      0.05}}},
    {rr_mob_id_lanternfly,         rr_rarity_id_common, rr_rarity_id_ultimate,  20, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_max,    rr_rarity_id_max,     {{rr_petal_id_mint,     0.1},{rr_petal_id_sand,      0.05}}},
};

char const *RR_MOB_NAMES[rr_mob_id_max] = {
"Triceratops","T-Rex","Fern","Tree","Pteranodon","Dakotaraptor",
"Pachycephalosaurus","Ornithomimus","Ankylosaurus","Meteor",
"Quetzalcoatlus","Edmontosaurus","Ant","Hornet","Dragonfly",
"Honeybee","Beehive","Spider","House Centipede","Lanternfly"
};

uint32_t RR_MOB_DIFFICULTY_COEFFICIENTS[rr_mob_id_max] = {
    3, //tric
    4, //trex
    1, //fern
    2, //tree
    5, //pter
    5, //dako
    3, //pachy
    2, //ornith
    4, //anky
    1, //meteor
    5, //quetz
    3, //edmo
};

double RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max] = {
    50,   //tric
    100,  //trex
    15,   //fern
    0.25, //tree
    75,   //pter
    50,   //dako
    25,   //pachy
    40,   //ornith
    25,   //anky
    1,    //meteor
    75,   //quetz
    25,   //edmo
};
double RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 10};

struct rr_petal_rarity_scale RR_PETAL_RARITY_SCALE[rr_rarity_id_max] = {
    {1,    240, 45 },
    {1.8,  120, 60 },
    {3.5,  60,  75 },
    {6.8,  30,  100},
    {12.5, 15,  125},
    {24.5, 7.5, 150},
    {60,   2.5, 200},
    {180,  0.5, 250},
};

struct rr_mob_rarity_scale RR_MOB_RARITY_SCALING[rr_rarity_id_max] = {
    {1,      1,    1  },
    {2.4,    1.7,  1.2},
    {6,      2.9,  1.5}, 
    {14.4,   5,    2  },
    {40,     8.5,  2.8},
    {192,    14.5, 4  },
    {2560,   24.6, 5.5},
    {128000, 42,   7  },
};
// clang-format on

uint32_t RR_RARITY_COLORS[rr_rarity_id_max] = {
    0xff7eef6d, 0xffffe65d, 0xff4d52e3, 0xff861fde,
    0xffde1f1f, 0xff1fdbde, 0xffff2b75, 0xff2bffa3};

char const *RR_RARITY_NAMES[rr_rarity_id_max] = {
    "Common",    "Unusual", "Rare",   "Epic",
    "Legendary", "Mythic",  "Exotic", "Ancient"};

double RR_MOB_WAVE_RARITY_COEFFICIENTS[rr_rarity_id_max + 1] = {
    0, 1, 6, 10, 15, 25, 160, 1200, 250};

double RR_DROP_RARITY_COEFFICIENTS[rr_rarity_id_exotic + 2] = {
    0, 1, 8, 15, 40, 150, 500, 2500};
double RR_MOB_LOOT_RARITY_COEFFICIENTS[rr_rarity_id_max] = {2.5, 4,  6,   15,
                                                            35,  50, 125, 150};

static void init_game_coefficients()
{
    double sum = 1;
    double sum2 = 1;
    for (uint64_t a = 1; a < rr_rarity_id_max; ++a)
        RR_MOB_LOOT_RARITY_COEFFICIENTS[a] *=
            RR_MOB_LOOT_RARITY_COEFFICIENTS[a - 1];
    for (uint64_t a = 1; a <= rr_rarity_id_exotic; ++a)
    {
        sum += (RR_DROP_RARITY_COEFFICIENTS[a + 1] =
                    RR_DROP_RARITY_COEFFICIENTS[a] /
                    RR_DROP_RARITY_COEFFICIENTS[a + 1]);
    }
    for (uint64_t a = 1; a <= rr_rarity_id_exotic + 1; ++a)
    {
        RR_DROP_RARITY_COEFFICIENTS[a] = RR_DROP_RARITY_COEFFICIENTS[a] / sum +
                                         RR_DROP_RARITY_COEFFICIENTS[a - 1];
    }
    RR_DROP_RARITY_COEFFICIENTS[rr_rarity_id_exotic + 1] = 1;
    for (uint64_t a = 1; a <= rr_rarity_id_ultimate; ++a)
    {
        sum2 += (RR_MOB_WAVE_RARITY_COEFFICIENTS[a + 1] =
                     RR_MOB_WAVE_RARITY_COEFFICIENTS[a] /
                     RR_MOB_WAVE_RARITY_COEFFICIENTS[a + 1]);
    }
    for (uint64_t a = 1; a <= rr_rarity_id_ultimate + 1; ++a)
    {
        RR_MOB_WAVE_RARITY_COEFFICIENTS[a] =
            RR_MOB_WAVE_RARITY_COEFFICIENTS[a] / sum2 +
            RR_MOB_WAVE_RARITY_COEFFICIENTS[a - 1];
    }
    RR_MOB_WAVE_RARITY_COEFFICIENTS[rr_rarity_id_ultimate + 1] = 1;
    for (uint64_t mob = 1; mob < rr_mob_id_max; ++mob)
    {
        RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[mob] +=
            RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[mob - 1];
        RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[mob] +=
            RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[mob - 1];
    }
    for (uint64_t mob = 0; mob < rr_mob_id_max; ++mob)
    {
        RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[mob] /=
            RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max - 1];
        RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[mob] /=
            RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max - 1];
    }
}

#define offset(a, b)                                                           \
    ((x + a < 0 || y + b < 0 || x + a >= size / 2 || y + b >= size / 2)        \
         ? 0                                                                   \
         : template[(y + b) * size / 2 + x + a])
#define maze_grid(x, y) maze[(y)*size + (x)]

static void init_maze(uint32_t size, uint8_t *template,
                      struct rr_maze_grid *maze)
{
    for (int32_t y = 0; y < size / 2; ++y)
    {
        for (int32_t x = 0; x < size / 2; ++x)
        {
            uint8_t this_tile = offset(0, 0);
#ifdef RR_SERVER
            maze_grid(x * 2, y * 2).difficulty = this_tile;
            maze_grid(x * 2 + 1, y * 2).difficulty = this_tile;
            maze_grid(x * 2, y * 2 + 1).difficulty = this_tile;
            maze_grid(x * 2 + 1, y * 2 + 1).difficulty = this_tile;
#endif
            this_tile = this_tile != 0;
            // top left
            uint8_t top = offset(0, -1);
            uint8_t bottom = offset(0, 1);
            if (this_tile)
            {
                if (top == 0)
                {
                    if (offset(-1, 0) == 0)
                        maze_grid(x * 2, y * 2).value = 7;
                    else
                        maze_grid(x * 2, y * 2).value = this_tile;
                    if (offset(1, 0) == 0)
                        maze_grid(x * 2 + 1, y * 2).value = 5;
                    else
                        maze_grid(x * 2 + 1, y * 2).value = this_tile;
                }
                else
                {
                    maze_grid(x * 2, y * 2).value = this_tile;
                    maze_grid(x * 2 + 1, y * 2).value = this_tile;
                }
                if (bottom == 0)
                {
                    if (offset(-1, 0) == 0)
                        maze_grid(x * 2, y * 2 + 1).value = 6;
                    else
                        maze_grid(x * 2, y * 2 + 1).value = this_tile;
                    if (offset(1, 0) == 0)
                        maze_grid(x * 2 + 1, y * 2 + 1).value = 4;
                    else
                        maze_grid(x * 2 + 1, y * 2 + 1).value = this_tile;
                }
                else
                {
                    maze_grid(x * 2, y * 2 + 1).value = this_tile;
                    maze_grid(x * 2 + 1, y * 2 + 1).value = this_tile;
                }
            }
            else
            {
                if (top)
                {
                    if (offset(-1, 0) && offset(-1, -1))
                        maze_grid(x * 2, y * 2).value = 15;
                    else
                        maze_grid(x * 2, y * 2).value = this_tile;
                    if (offset(1, 0) && offset(1, -1))
                        maze_grid(x * 2 + 1, y * 2).value = 13;
                    else
                        maze_grid(x * 2 + 1, y * 2).value = this_tile;
                }
                else
                {
                    maze_grid(x * 2, y * 2).value = this_tile;
                    maze_grid(x * 2 + 1, y * 2).value = this_tile;
                }
                if (bottom)
                {
                    if (offset(-1, 0) && offset(-1, 1))
                        maze_grid(x * 2, y * 2 + 1).value = 14;
                    else
                        maze_grid(x * 2, y * 2 + 1).value = this_tile;
                    if (offset(1, 0) && offset(1, 1))
                        maze_grid(x * 2 + 1, y * 2 + 1).value = 12;
                    else
                        maze_grid(x * 2 + 1, y * 2 + 1).value = this_tile;
                }
                else
                {
                    maze_grid(x * 2, y * 2 + 1).value = this_tile;
                    maze_grid(x * 2 + 1, y * 2 + 1).value = this_tile;
                }
            }
        }
    }
}

static void print_chances(float difficulty)
{
    printf("-----Chances for %.0f-----\n", difficulty);
    uint32_t rarity_cap = rr_rarity_id_common + (difficulty + 7) / 8;
    if (rarity_cap > rr_rarity_id_ultimate)
        rarity_cap = rr_rarity_id_ultimate;
    uint32_t rarity = rarity_cap >= 2 ? rarity_cap - 2 : 0;
    for (; rarity <= rarity_cap; ++rarity)
    {
        float start =
            rarity == 0
                ? 0
                : pow(1 - (1 - RR_MOB_WAVE_RARITY_COEFFICIENTS[rarity]) * 0.3,
                      pow(1.5, difficulty));
        float end =
            rarity == rarity_cap
                ? 1
                : pow(1 - (1 - RR_MOB_WAVE_RARITY_COEFFICIENTS[rarity + 1]) *
                              0.3,
                      pow(1.5, difficulty));
        printf("%s: %.9f (1 per %.4f)\n", RR_RARITY_NAMES[rarity], end - start,
               1 / (end - start));
    }
}

double RR_BASE_CRAFT_CHANCES[rr_rarity_id_max - 1] = {0.5,  0.3,  0.15, 0.05,
                                                      0.03, 0.02, 0.015};
double RR_CRAFT_CHANCES[rr_rarity_id_max - 1];

static double from_prd_base(double C)
{
    double pProcOnN = 0;
    double pProcByN = 0;
    double sumNpProcOnN = 0;

    double maxFails = ceil(1 / C);
    for (uint32_t N = 1; N <= maxFails; ++N)
    {
        pProcOnN = fmin(1, N * C) * (1 - pProcByN);
        pProcByN += pProcOnN;
        sumNpProcOnN += N * pProcOnN;
    }
    return (1 / sumNpProcOnN);
}

static double get_prd_base(double p)
{
    if (p == 0)
        return 0;
    double Cupper = p;
    double Clower = 0;
    double Cmid = p / 2;
    double p1 = 0;
    double p2 = 1;
    while (1)
    {
        Cmid = (Cupper + Clower) / 2;
        p1 = from_prd_base(Cmid);
        if (fabs(p1 - p2) <= 0)
            break;

        if (p1 > p)
            Cupper = Cmid;
        else
            Clower = Cmid;
        p2 = p1;
    }
    return Cmid;
}

#define init(MAZE)                                                             \
    init_maze(sizeof(RR_MAZE_##MAZE[0]) / sizeof(struct rr_maze_grid),         \
              &RR_MAZE_TEMPLATE_##MAZE[0][0], &RR_MAZE_##MAZE[0][0]);

void rr_static_data_init()
{
    for (uint32_t r = 0; r < rr_rarity_id_max - 1; ++r)
        RR_CRAFT_CHANCES[r] = get_prd_base(RR_BASE_CRAFT_CHANCES[r]);
    init_game_coefficients();
    init(HELL_CREEK);
    init(BURROW);
#ifdef RR_SERVER
    print_chances(1);  // c
    print_chances(4);  // C
    print_chances(8);  // u
    print_chances(12); // U
    print_chances(16); // r
    print_chances(20); // R
    print_chances(24); // e
    print_chances(28); // E
    print_chances(32); // l
    print_chances(36); // L
    print_chances(40); // m
    print_chances(44); // M
    print_chances(48); // x
    print_chances(52); // X
    print_chances(56); // a
    print_chances(60); // A
#endif
}

double xp_to_reach_level(uint32_t level)
{
    if (level <= 60)
        return (level + 5) * pow(1.175, level);
    double base = (level + 5) * pow(1.175, 60);
    for (uint32_t i = 60; i < level; ++i)
        base *= rr_fclamp(1.18 - 0.0075 * (i - 60), 1.1, 1.18);
    return base;
}

uint32_t level_from_xp(double xp)
{
    uint32_t level = 1;
    while (xp >= xp_to_reach_level(level + 1))
        xp -= xp_to_reach_level(++level);
    return level;
}

#ifdef RR_SERVER
#define _ 0
#define c 1
#define C 4
#define u 8
#define U 12
#define r 16
#define R 20
#define e 24
#define E 28
#define l 32
#define L 36
#define m 40
#define M 44
#define x 48
#define X 52
#define a 56
#define A 60
#else
#define _ 0
#define c 1
#define C 1
#define u 1
#define U 1
#define r 1
#define R 1
#define e 1
#define E 1
#define l 1
#define L 1
#define m 1
#define M 1
#define x 1
#define X 1
#define a 1
#define A 1
#endif

#define RR_DEFINE_MAZE(name, size)                                             \
    struct rr_maze_grid RR_MAZE_##name[size][size];                            \
    uint8_t RR_MAZE_TEMPLATE_##name[size / 2][size / 2]
// clang-format off
RR_DEFINE_MAZE(HELL_CREEK, 80) = {
//                     11  13  15  17  19  21  23  25  27  29  31  33  35  37  39
// 1 2 3 4 5 6 7 8 9 10  12  14  16  18  20  22  24  26  28  30  32  34  36  38
{c,c,_,_,r,r,_,_,_,r,r,_,_,r,r,r,r,r,R,R,R,R,R,e,e,e,e,e,E,E,E,E,E,E,E,l,l,l,l,l}, // 0
{c,c,c,_,_,r,r,_,r,r,_,_,U,U,_,_,r,r,_,R,R,_,_,e,e,e,_,_,E,E,E,_,_,E,E,l,l,l,l,l}, // 1
{_,c,c,c,_,_,r,r,r,_,_,U,U,_,_,r,r,_,_,_,R,R,_,_,e,e,e,_,_,E,E,E,_,_,E,_,_,_,_,l}, // 2
{_,_,c,c,c,_,_,U,_,_,U,U,_,_,r,r,_,_,e,_,_,e,e,_,_,e,e,e,_,_,E,l,l,_,_,_,L,L,_,l}, // 3
{_,C,C,C,_,_,_,U,_,U,U,_,_,r,r,_,_,e,e,e,_,_,e,e,_,_,e,e,e,_,_,l,l,l,_,L,L,L,_,l}, // 4
{C,C,C,_,_,U,U,U,U,U,_,_,r,r,_,_,_,e,e,e,_,_,_,e,e,_,_,E,E,E,_,_,l,l,l,L,L,L,_,l}, // 5
{C,C,_,_,U,U,U,U,U,_,_,r,r,_,_,_,e,e,e,e,e,_,_,_,E,E,_,_,E,E,E,_,_,_,_,_,_,_,_,l}, // 6
{C,_,_,u,u,_,_,_,_,_,r,r,_,_,_,e,e,_,_,_,e,e,_,_,_,E,E,_,_,E,E,E,_,X,X,X,X,X,_,l}, // 7
{C,_,u,u,_,R,R,R,R,R,r,r,_,_,_,R,_,_,e,_,_,R,_,_,_,E,l,l,_,_,E,E,_,X,X,X,_,_,_,l}, // 8
{u,u,u,_,R,R,R,R,R,R,r,_,_,_,_,R,_,e,e,e,_,R,_,_,_,_,_,l,l,_,E,E,_,X,X,X,x,_,_,L}, // 9
{u,u,_,R,R,_,_,R,R,_,_,_,_,_,R,R,_,_,e,_,_,R,R,_,_,_,_,_,_,_,_,l,_,X,_,x,x,x,_,L}, // 10
{u,_,R,R,_,e,_,_,R,R,_,_,_,_,R,_,_,_,e,_,_,_,R,_,_,_,_,l,l,_,l,l,_,X,_,_,x,x,_,L}, // 11
{_,R,R,_,e,e,e,_,_,R,R,_,R,R,R,_,_,e,e,e,_,_,E,E,E,_,l,l,_,_,l,l,_,_,_,x,x,_,_,L}, // 12
{R,R,_,e,e,_,e,e,_,_,R,R,R,R,R,R,e,e,e,e,e,E,E,E,E,E,E,_,_,l,l,l,_,_,x,x,_,_,L,L}, // 13
{R,_,e,e,_,e,e,_,E,_,_,R,R,_,R,_,_,e,e,e,_,_,E,_,E,E,_,_,l,l,l,_,_,x,x,_,_,L,L,L}, // 14
{e,e,e,_,e,e,_,E,E,E,_,_,R,R,R,_,_,_,e,_,_,_,E,E,E,_,_,L,L,L,_,_,x,x,_,_,L,L,L,_}, // 15
{e,e,_,e,e,_,E,E,_,E,E,_,_,R,R,R,_,e,e,e,_,E,E,E,_,_,L,L,L,_,_,x,x,_,_,L,L,L,_,_}, // 16
{e,_,e,e,_,E,E,_,E,E,_,_,_,R,R,_,_,_,E,_,_,_,E,E,_,L,L,L,_,_,x,x,_,_,m,m,m,_,_,x}, // 17
{_,e,e,_,E,E,_,E,E,_,L,_,_,R,_,E,E,E,E,E,E,E,_,E,_,_,L,L,_,x,x,_,_,m,m,m,_,_,x,x}, // 18
{e,e,_,E,E,_,E,E,_,L,L,_,R,R,_,E,E,_,_,_,E,E,_,E,E,_,L,L,_,x,x,_,m,m,m,_,_,x,x,x}, // 19
{e,_,E,E,_,l,l,_,L,L,_,_,R,_,E,E,_,_,M,_,_,E,E,_,E,_,_,L,_,_,x,_,_,m,m,_,x,x,x,x}, // 20
{E,E,E,_,l,l,_,L,L,L,_,e,e,e,e,_,_,m,L,c,_,_,l,l,l,l,_,L,L,_,x,x,_,_,_,_,x,x,x,M}, // 21
{E,E,_,l,l,_,L,L,L,_,_,e,e,e,_,_,e,l,_,r,E,_,_,l,l,l,_,_,L,_,_,x,x,_,_,x,x,_,_,M}, // 22
{E,_,l,l,_,l,l,_,L,_,e,e,e,_,_,r,L,_,_,_,C,e,_,_,l,l,l,_,L,L,_,_,x,x,x,x,_,_,M,M}, // 23
{_,l,l,_,l,l,_,L,L,_,e,e,_,_,R,E,_,_,_,_,_,l,m,_,_,l,l,_,L,L,L,_,_,x,x,_,_,M,M,M}, // 24
{l,l,_,l,l,_,L,L,_,_,_,_,_,M,C,_,_,M,_,M,_,_,M,U,_,_,_,_,L,_,_,_,_,_,_,_,_,_,_,M}, // 25
{l,_,l,l,_,L,L,_,m,m,_,_,l,U,_,_,M,M,M,M,M,_,_,R,u,_,_,_,m,m,m,m,_,m,m,m,_,m,m,M}, // 26
{l,l,l,_,L,L,_,m,m,m,m,r,L,M,M,M,M,_,M,_,M,M,M,M,E,c,_,_,m,m,_,m,m,m,_,m,m,m,m,M}, // 27
{l,l,_,L,L,_,m,m,_,_,R,E,_,M,M,_,M,M,M,M,M,_,M,M,_,e,M,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 28
{l,_,L,L,_,m,m,_,_,u,C,_,_,M,M,M,M,_,M,_,M,M,M,M,_,_,C,L,_,_,a,a,a,a,a,a,a,a,a,a}, // 29
{_,m,m,m,m,m,_,_,c,M,_,_,_,M,_,M,M,M,M,M,M,M,_,M,_,_,_,m,M,_,a,_,_,_,_,_,_,_,_,a}, // 30
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,x,_,a,_,a,a,a,a,a,a,_,a}, // 31
{x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,_,a,_,a,_,_,_,_,a,_,a}, // 32
{x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,x,x,x,_,a,_,a,_,A,A,_,a,_,a}, // 33
{x,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,a,_,a,_,A,A,_,a,_,a}, // 34
{X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,a,_,a,a,a,a,_,a,_,a}, // 35
{X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,a,_,_,_,_,_,_,a,_,a}, // 36
{X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,a,a,a,a,a,a,a,a,_,a}, // 37
{X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,_,_,_,_,_,_,_,_,_,a}, // 38
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,a,a,a,a,a,a,a,a,a,a,a,a,a}, // 39
};
// clang-format on
RR_DEFINE_MAZE(BURROW, 4) = {{1, 1}, {0, 1}};

#define MAZE_ENTRY(MAZE, GRID_SIZE)                                            \
    (sizeof(RR_MAZE_##MAZE[0]) / sizeof(struct rr_maze_grid)), GRID_SIZE,      \
        &RR_MAZE_##MAZE[0][0]

struct rr_maze_declaration RR_MAZES[rr_biome_id_max] = {
    {MAZE_ENTRY(HELL_CREEK, 1024), 11, {
        {0,  0,  3, 3, 1,  1,  1},   // 0
        {6,  4,  3, 3, 7,  5,  20},  // 1
        {16, 0,  5, 2, 18, 0,  40},  // 2
        {5,  8,  5, 2, 7,  9,  60},  // 3
        {17, 12, 3, 3, 18, 13, 80},  // 4
        {16, 5,  5, 2, 18, 13, 80},  // 5
        {12, 19, 3, 3, 18, 13, 80},  // 6
        {28, 0,  3, 2, 29, 1,  60},  // 7
        {38, 13, 2, 2, 38, 14, 80},  // 8
        {26, 16, 2, 3, 26, 17, 80},  // 9
        {8,  20, 2, 5, 8,  22, 100}, // 10
    }},
    {MAZE_ENTRY(HELL_CREEK, 1024), 0},
    {MAZE_ENTRY(BURROW, 512), 0},
};

uint8_t RR_GLOBAL_BIOME = rr_biome_id_hell_creek;
#undef _
#undef c
#undef C
#undef u
#undef U
#undef r
#undef R
#undef e
#undef E
#undef l
#undef L
#undef m
#undef M
#undef x
#undef X
#undef a
#undef A
