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
//   hp             dmg
    {1,            1                }, // rr_rarity_id_common,
    {1.7,          2                }, // rr_rarity_id_unusual,
    {2.9,          4                }, // rr_rarity_id_rare,
    {5,            8                }, // rr_rarity_id_epic,
    {8.5,          16               }, // rr_rarity_id_legendary,
    {14.5,         48               }, // rr_rarity_id_mythic,
    {24.6,         144              }, // rr_rarity_id_exotic,
    {42,           432              }, // rr_rarity_id_ultimate,
    {63,           648              }, // rr_rarity_id_quantum,
    {94.5,         972              }, // rr_rarity_id_aurous,
    {141.75,       1458             }, // rr_rarity_id_eternal,
    {255.15,       3936.6           }, // rr_rarity_id_hyper,
    {510.3,        7873.2           }, // rr_rarity_id_sunshine,
    {714.42,       11809.8          }, // rr_rarity_id_nebula,
    {1071.63,      16533.72         }, // rr_rarity_id_infinity,
    {1500.282,     23147.208        }, // rr_rarity_id_calamity,
    {3750.705,     34720.812        }, // rr_rarity_id_unique,
    {7876.2805,    83329.9488       }, // rr_rarity_id_cosmic,
    {10239.42465,  104192.436       }, // rr_rarity_id_galactic
};

struct rr_petal_base_stat_scale const defensive[rr_rarity_id_max] = {
//   hp                dmg
    {1,                1            }, // rr_rarity_id_common,
    {2,                1.7          }, // rr_rarity_id_unusual,
    {4,                2.9          }, // rr_rarity_id_rare,
    {8,                5            }, // rr_rarity_id_epic,
    {16,               8.5          }, // rr_rarity_id_legendary,
    {48,               14.5         }, // rr_rarity_id_mythic,
    {144,              24.6         }, // rr_rarity_id_exotic,
    {432,              42           }, // rr_rarity_id_ultimate,
    {648,              63           }, // rr_rarity_id_quantum,
    {972,              94.5         }, // rr_rarity_id_aurous,
    {1458,             141.75       }, // rr_rarity_id_eternal,
    {3936.6,           255.15       }, // rr_rarity_id_hyper,
    {7873.2,           510.3        }, // rr_rarity_id_sunshine,
    {11809.8,          714.42       }, // rr_rarity_id_nebula,
    {16533.72,         1071.63      }, // rr_rarity_id_infinity,
    {23147.208,        1500.282     }, // rr_rarity_id_calamity,
    {34720.812,        3750.705     }, // rr_rarity_id_unique,
    {83329.9488,       7876.2805    }, // rr_rarity_id_cosmic,
    {104192.436,       10239.42465  }, // rr_rarity_id_galactic
};

struct rr_petal_data RR_PETAL_DATA[rr_petal_id_max] = {
//   id                         min_rarity              scale          dmg        hp   clump     cd   2cd  count
    {rr_petal_id_none,          rr_rarity_id_common,    offensive,    0.0f,     0.0f,   0.0f,     0,    0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0}},
    {rr_petal_id_basic,         rr_rarity_id_common,    offensive,   10.0f,    15.0f,   0.0f,    25,    0, {1,1,1,1,1,1,1,2,3,3,3,4,4,5,5, 6, 6, 6, 6}},
    {rr_petal_id_pellet,        rr_rarity_id_common,    offensive,   14.4f,     5.0f,   0.0f,    13,    0, {1,2,2,3,3,3,5,5,6,6,6,6,8,8,10,12,24,24,24}},
    {rr_petal_id_fossil,        rr_rarity_id_common,    offensive,    5.0f,   125.0f,   0.0f,   100,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,2,2, 2, 3, 3, 3}},
    {rr_petal_id_stinger,       rr_rarity_id_common,    offensive,   70.0f,     3.0f,  10.0f,   150,    0, {1,1,1,1,1,3,4,5,5,6,6,6,6,6,6, 6, 6, 6, 6}},
    {rr_petal_id_berry,         rr_rarity_id_rare,      offensive,    5.0f,     5.0f,  12.0f,    13,    0, {1,1,1,1,1,1,1,2,3,3,3,3,3,3,3, 3, 5, 5, 5}},
    {rr_petal_id_shell,         rr_rarity_id_rare,      offensive,   18.0f,    16.0f,   0.0f,    50,   13, {1,1,1,1,1,2,3,3,3,3,3,3,3,3,3, 5, 5, 5, 5}},
    {rr_petal_id_peas,          rr_rarity_id_rare,      offensive,   22.0f,    12.0f,   8.0f,    13,   12, {4,4,4,4,4,4,4,4,5,5,5,5,5,5,5, 6, 6, 8, 8}},
    {rr_petal_id_leaf,          rr_rarity_id_unusual,   offensive,    9.0f,     8.0f,   8.0f,    38,    0, {1,1,1,1,1,1,1,1,1,3,3,3,3,5,5, 5, 5, 5, 5}},
    {rr_petal_id_egg,           rr_rarity_id_unusual,   defensive,    0.0f,    75.0f,  10.0f,    25,  100, {3,3,3,2,2,2,2,1,1,1,1,1,1,1,1, 1, 2, 1, 1}},
    {rr_petal_id_magnet,        rr_rarity_id_common,    defensive,    2.0f,    25.0f,   0.0f,    38,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2, 2, 3, 3}},
    {rr_petal_id_uranium,       rr_rarity_id_rare,      offensive,    3.0f,    40.0f,   0.0f,    25,   25, {1,1,1,1,1,1,1,2,2,2,2,2,2,3,3, 3, 3, 3, 3}},
    {rr_petal_id_feather,       rr_rarity_id_common,    defensive,    1.0f,     3.0f,   0.0f,    25,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,2,2, 2, 2, 3, 3}},
    {rr_petal_id_azalea,        rr_rarity_id_common,    defensive,    5.0f,    15.0f,  10.0f,    25,   50, {1,1,1,1,1,1,3,3,3,3,3,5,5,5,5, 6, 6, 6, 6}},
    {rr_petal_id_bone,          rr_rarity_id_common,    defensive,    2.5f,    25.0f,   0.0f,    68,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 3, 3, 3}},
    {rr_petal_id_web,           rr_rarity_id_rare,      defensive,    5.0f,     5.0f,   0.0f,    50,   13, {1,1,1,1,1,1,1,1,1,1,1,3,3,3,3, 3, 3, 3, 3}},
    {rr_petal_id_seed,          rr_rarity_id_legendary, defensive,    1.0f,    75.0f,   0.0f,    63,    1, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 3, 3, 3}},
    {rr_petal_id_gravel,        rr_rarity_id_unusual,   offensive,    9.0f,    20.0f,   0.0f,    20,   10, {1,2,2,2,3,3,3,3,3,3,3,3,5,5,6, 6, 6, 8, 8}},
    {rr_petal_id_club,          rr_rarity_id_common,    defensive,    8.0f,   600.0f,   0.0f,   250,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 2, 3, 3}},
    {rr_petal_id_crest,         rr_rarity_id_rare,      offensive,    0.0f,     0.0f,   0.0f,     0,    0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0}},
    {rr_petal_id_droplet,       rr_rarity_id_common,    offensive,   15.0f,     5.0f,   0.0f,    37,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_beak,          rr_rarity_id_unusual,   defensive,    0.0f,    10.0f,   0.0f,    55,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,3,3, 3, 3, 3, 3}},
    {rr_petal_id_lightning,     rr_rarity_id_unusual,   offensive,  13.65f,     1.0f,   0.0f,    63,    0, {1,1,1,1,1,1,1,1,1,1,2,2,2,3,3, 3, 3, 5, 6}},
    {rr_petal_id_third_eye,     rr_rarity_id_legendary, offensive,    0.0f,     0.0f,   0.0f,     0,    0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0}},
    {rr_petal_id_nest,          rr_rarity_id_legendary, defensive,    5.0f,    25.0f,   0.0f,   125,    1, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_fireball,      rr_rarity_id_unusual,   offensive,  290.0f,     1.0f,   0.0f,   600,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 3, 3}},
    {rr_petal_id_meat,          rr_rarity_id_common,    offensive,    0.0f,  1600.0f,   0.0f,   188,   13, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 3, 3}},
    {rr_petal_id_bubble,        rr_rarity_id_common,    defensive,    1.0f,    25.0f,   0.0f,    88,    3, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_meteor,        rr_rarity_id_unusual,   defensive,    0.0f,     0.0f,   0.0f,    50,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_mandible,      rr_rarity_id_common,    offensive,    5.0f,    10.0f,   0.0f,    75,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,2,2, 3, 3, 3, 3}},
    {rr_petal_id_wax,           rr_rarity_id_unusual,   offensive,   10.0f,    10.0f,  10.0f,    38,    0, {2,2,2,2,2,2,2,2,2,3,3,3,3,3,3, 5, 5, 6, 6}},
    {rr_petal_id_sand,          rr_rarity_id_common,    offensive,   15.0f,    10.0f,  10.0f,    37,    0, {4,4,4,4,4,4,4,4,4,4,4,5,5,6,6, 8, 10,12,12}},
    {rr_petal_id_mint,          rr_rarity_id_unusual,   offensive,    5.0f,    10.0f,  10.0f,    50,   25, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_pearl,         rr_rarity_id_exotic,    offensive, 1012.5f,   472.5f, -25.0f,  1500,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 2, 2}},
    {rr_petal_id_rice,          rr_rarity_id_common,    offensive,    2.0f,     0.1f,  10.0f,     1,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 3, 3}},
    {rr_petal_id_sapphire,      rr_rarity_id_eternal,   offensive,    0.0f,   350.0f,  15.0f,   500,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 2, 2, 2}},
    {rr_petal_id_ruby,          rr_rarity_id_eternal,   offensive,    0.0f,    35.0f,  15.0f,   138,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_golden_leaf,   rr_rarity_id_mythic,    offensive,    2.5f,     1.0f,  10.0f,    20,    0, {1,1,1,1,1,1,1,1,1,3,3,3,3,3,3, 3, 5, 5, 5}},
    {rr_petal_id_diamond_leaf,  rr_rarity_id_aurous,    offensive,    7.7f,     1.0f,  10.0f,    13,    0, {1,1,1,1,1,1,1,1,1,3,3,3,3,3,3, 3, 5, 5, 5}},
    {rr_petal_id_emerald_leaf,  rr_rarity_id_hyper,     offensive,   35.0f,    28.0f,  10.0f,     7,    0, {1,1,1,1,1,1,1,1,1,3,3,3,3,3,3, 3, 5, 5, 5}},
    {rr_petal_id_amethyst_leaf, rr_rarity_id_nebula,    offensive,   85.5f,    36.0f,  10.0f,     1,    0, {1,1,1,1,1,1,1,1,1,3,3,3,3,3,3, 3, 5, 5, 5}},
    {rr_petal_id_diamond,       rr_rarity_id_eternal,   offensive, 2.5e25f, 1.0e-25f,   0.0f,  5500,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_golden_meteor, rr_rarity_id_mythic,    offensive,    0.0f,     0.0f,   0.0f,   100,    0, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1, 1, 1, 1}},
    {rr_petal_id_blood_stinger, rr_rarity_id_legendary, offensive,   70.0f,     3.0f,  10.0f,    75,    0, {1,1,1,1,1,3,4,5,5,6,6,6,6,6,6, 6, 6, 6, 6}},
};    

char const *RR_PETAL_NAMES[rr_petal_id_max] = {
    "Secret",        "Basic",         "Pellet",      "Fossil",        "Stinger",       "Berry",          "Shell",
    "Peas",          "Leaf",          "Egg",         "Magnet",        "Uranium",       "Feather",        "Azalea",
    "Bone",          "Web",           "Seed",        "Gravel",        "Club",          "Crest",          "Droplet",
    "Beak",          "Lightning",     "Third Eye",   "Nest",          "Fireball",      "Meat",           "Bubble",
    "Meteor",        "Mandible",      "Wax",         "Sand",          "Mint",          "Pearl",          "Rice",
    "Sapphire",      "Ruby",          "Leaf",        "Leaf",          "Leaf",          "Leaf",           "Diamond",
    "Meteor",        "Stinger"
};

char const *RR_PETAL_NAMES_FULL[rr_petal_id_max] = {
    "Secret",        "Basic",         "Pellet",      "Fossil",        "Stinger",       "Berry",          "Shell",
    "Peas",          "Leaf",          "Egg",         "Magnet",        "Uranium",       "Feather",        "Azalea",
    "Bone",          "Web",           "Seed",        "Gravel",        "Club",          "Crest",          "Droplet",
    "Beak",          "Lightning",     "Third Eye",   "Nest",          "Fireball",      "Meat",           "Bubble",
    "Meteor",        "Mandible",      "Wax",         "Sand",          "Mint",          "Pearl",          "Rice",
    "Sapphire",      "Ruby",          "Golden Leaf", "Diamond Leaf",  "Emerald Leaf",  "Amethyst Leaf",  "Diamond",
    "Golden Meteor", "Blood Stinger"
};
    
char const *RR_PETAL_DESCRIPTIONS[rr_petal_id_max] = {
    0,
    "It's just a petal",
    "Low damage, but there's lots",
    "It came from a dino",
    "Ow that hurts",
    "Gives your other petals more energy",
    "Poor snail",
    "Splits in 4. Or maybe 5 if you're a pro. Or maybe more if...",
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
    "Remember to feed your pets",
    "So massive damage",
    "VERY FAST",
    "Permanent stun mob",
    "Dont work :<",
    "Faster reload",
    "More damage but less reload speed than golden",
    "-",
    "-",
    "So sharpness",
    "Oh, shiny!"
};

struct rr_mob_data RR_MOB_DATA[rr_mob_id_max] = {
//   id                                     min_rarity  max_rarity              hp dmg    rad  ai_passive_rarity    ai_neutral_rarity     ai_aggro_rarity       loot
    {rr_mob_id_triceratops,        rr_rarity_id_common, rr_rarity_id_galactic,  45, 15, 30.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_max,     {{rr_petal_id_leaf,    0.15},{rr_petal_id_fossil,        0.05},{rr_petal_id_golden_leaf,       0.003}}},
    {rr_mob_id_trex,               rr_rarity_id_common, rr_rarity_id_galactic,  40, 25, 32.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_unusual, {{rr_petal_id_stinger, 0.03},{rr_petal_id_egg,           0.05},{rr_petal_id_meat,               0.01}}},
    {rr_mob_id_fern,               rr_rarity_id_common, rr_rarity_id_galactic,  10,  5, 24.0f, rr_rarity_id_max,    rr_rarity_id_max,     rr_rarity_id_max,     {{rr_petal_id_leaf,     0.1},{rr_petal_id_azalea,        0.25},{rr_petal_id_sapphire,         0.0005},{rr_petal_id_golden_leaf, 0.002}}},
    {rr_mob_id_tree,               rr_rarity_id_common, rr_rarity_id_galactic, 100,  5, 64.0f, rr_rarity_id_max,    rr_rarity_id_max,     rr_rarity_id_max,     {{rr_petal_id_leaf,     2.5},{rr_petal_id_peas,           2.5},{rr_petal_id_seed,               0.05},{rr_petal_id_golden_leaf,  0.02}}},
    {rr_mob_id_pteranodon,         rr_rarity_id_common, rr_rarity_id_galactic,  40, 15, 20.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_rare,    {{rr_petal_id_shell,   0.05},{rr_petal_id_beak,          0.15},{rr_petal_id_nest,               0.01},{rr_petal_id_ruby,        0.005}}},
    {rr_mob_id_dakotaraptor,       rr_rarity_id_common, rr_rarity_id_galactic,  35, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_epic,    {{rr_petal_id_crest,    0.1},{rr_petal_id_feather,        0.1},{rr_petal_id_pellet,             0.05}}},
    {rr_mob_id_pachycephalosaurus, rr_rarity_id_common, rr_rarity_id_galactic,  35, 20, 20.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_common,  {{rr_petal_id_fossil,   0.1},{rr_petal_id_berry,          0.1},{rr_petal_id_web,                0.05}}},
    {rr_mob_id_ornithomimus,       rr_rarity_id_common, rr_rarity_id_galactic,  25, 10, 20.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_max,     {{rr_petal_id_feather,  0.1},{rr_petal_id_droplet,       0.05},{rr_petal_id_pellet,              0.1}}},
    {rr_mob_id_ankylosaurus,       rr_rarity_id_common, rr_rarity_id_galactic,  50, 10, 30.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_max,     {{rr_petal_id_club,    0.15},{rr_petal_id_gravel,        0.05},{rr_petal_id_bubble,              0.1},{rr_petal_id_pearl,       0.005}}},
    {rr_mob_id_meteor,             rr_rarity_id_common, rr_rarity_id_galactic,  40, 15, 32.0f, rr_rarity_id_common, rr_rarity_id_max,     rr_rarity_id_max,     {{rr_petal_id_magnet,   0.5},{rr_petal_id_uranium,       0.05},{rr_petal_id_fireball,            1.0},{rr_petal_id_meteor,        2.0}}},
    {rr_mob_id_quetzalcoatlus,     rr_rarity_id_common, rr_rarity_id_galactic,  65, 20, 28.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_common,  {{rr_petal_id_beak,    0.05},{rr_petal_id_fossil,         0.1},{rr_petal_id_lightning,          0.01}}},
    {rr_mob_id_edmontosaurus,      rr_rarity_id_common, rr_rarity_id_galactic,  50, 15, 30.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_max,     {{rr_petal_id_bone,    0.01},{rr_petal_id_fossil,         0.1},{rr_petal_id_third_eye,          0.05}}},

    {rr_mob_id_ant,                rr_rarity_id_common, rr_rarity_id_galactic,  10, 10, 20.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_rare,    {{rr_petal_id_pellet,   0.1},{rr_petal_id_leaf,           0.1},{rr_petal_id_mandible,           0.05},{rr_petal_id_rice,          0.5}}},
    {rr_mob_id_hornet,             rr_rarity_id_common, rr_rarity_id_galactic,  28, 25, 25.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_common,  {{rr_petal_id_stinger,  0.1},{rr_petal_id_crest,         0.05}}},
    {rr_mob_id_dragonfly,          rr_rarity_id_common, rr_rarity_id_galactic,  20, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_unusual, {{rr_petal_id_pellet,   0.1},{rr_petal_id_magnet,        0.05}}},
    {rr_mob_id_honeybee,           rr_rarity_id_common, rr_rarity_id_galactic,  12, 25, 22.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_exotic,  {{rr_petal_id_wax,     0.05},{rr_petal_id_stinger,       0.05},{rr_petal_id_blood_stinger,     0.0005}}},
    {rr_mob_id_beehive,            rr_rarity_id_common, rr_rarity_id_galactic,   0,  0, 45.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_max,     {{rr_petal_id_wax,     0.05},{rr_petal_id_azalea,        0.05}}},
    {rr_mob_id_spider,             rr_rarity_id_common, rr_rarity_id_galactic,  20, 25, 25.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_unusual, {{rr_petal_id_web,      0.1},{rr_petal_id_third_eye,     0.01}}},
    {rr_mob_id_house_centipede,    rr_rarity_id_common, rr_rarity_id_galactic,  25, 10, 23.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_ultimate,{{rr_petal_id_peas,     0.1},{rr_petal_id_sand,          0.05}}},
    {rr_mob_id_lanternfly,         rr_rarity_id_common, rr_rarity_id_galactic,  20, 10, 25.0f, rr_rarity_id_common, rr_rarity_id_common,  rr_rarity_id_ultimate,{{rr_petal_id_mint,     0.1},{rr_petal_id_sand,          0.05}}},
};

char const *RR_MOB_NAMES[rr_mob_id_max] = {
"Triceratops",   "T-Rex",            "Fern",               "Tree",
"Pteranodon",    "Dakotaraptor",     "Pachycephalosaurus", "Ornithomimus",
"Ankylosaurus",  "Meteor",           "Quetzalcoatlus",     "Edmontosaurus",
"Ant","Hornet",  "Dragonfly",        "Honeybee",           "Beehive",
"Spider",        "House Centipede",  "Lanternfly",
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
    1, //ant
    3, //hornet
    2, //dragonfly
    5, //honeybee
    2, //beehive
    4, //spider
    1, //house centi
    2, //lanternfly
};

double RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max] = {
    50,   //tric
    100,  //trex
    15,   //fern
    0.75, //tree
    75,   //pter
    50,   //dako
    25,   //pachy
    40,   //ornith
    25,   //anky
    0.75, //meteor
    75,   //quetz
    25,   //edmo
    20,   //ant
    20,   //hornet
    25,   //dragonfly
    10,   //honeybee
    7.5,  //beehive
    25,   //spider
    15,   //houseCenti
    15,   //lanternfly
};
double RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS[rr_mob_id_max] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 10};

struct rr_petal_rarity_scale RR_PETAL_RARITY_SCALE[rr_rarity_id_max] = {
//   heal   seed reload  web size   self damage
    {1,     480,         45,        1.1       }, // rr_rarity_id_common,
    {1.8,   240,         60,        1.98      }, // rr_rarity_id_unusual,
    {3.5,   120,          75,        3.85     }, // rr_rarity_id_rare,
    {6.8,   60,          100,       7.48      }, // rr_rarity_id_epic,
    {12.5,  30,          125,       13.75     }, // rr_rarity_id_legendary,
    {24.5,  15,         150,       26.95      }, // rr_rarity_id_mythic,
    {60,    7.5,         200,       66        }, // rr_rarity_id_exotic,
    {180,   5.0,         250,       198       }, // rr_rarity_id_ultimate,
    {240,   4.0,         300,       264       }, // rr_rarity_id_quantum,
    {390,   3.0,         325,       429       }, // rr_rarity_id_aurous,
    {510,   2.0,         350,       561       }, // rr_rarity_id_eternal,
    {850,   1.8,         375,       891       }, // rr_rarity_id_hyper,
    {1010,  1.6,         400,       1111      }, // rr_rarity_id_sunshine,
    {1200,  1.4,         450,       1320      }, // rr_rarity_id_nebula,
    {1640,  1.2,         475,       1804      }, // rr_rarity_id_infinity,
    {2800,  1.0,         500,       3080      }, // rr_rarity_id_calamity,
    {4000,  0.8,         525,       4400      }, // rr_rarity_id_unique,
    {7800,  0.4,         550,       8580      }, // rr_rarity_id_cosmic,
    {12400, 0.1,         575,       13640     }, // rr_rarity_id_galactic
};

struct rr_mob_rarity_scale RR_MOB_RARITY_SCALING[rr_rarity_id_max] = {
//  {hp              dmg            size                       },
    {1,              1,             1                          },
    {2.4,            1.7,           1.2                        },
    {6,              2.9,           1.5                        }, 
    {14.4,           5,             2                          },
    {40,             8.5,           2.8                        },
    {192,            14.5,          4                          },
    {2560,           24.6,          5.5                        }, // base rarity   base mult     hp mult  dmg mult  size mult
    {11946.2,        42,            7                          }, // ulti          ulti          x1.00    x1.00     x1.00
    {25087.02,       147,           8.4                        }, // quantum       ulti          x2.10    x3.50     x1.20
    {50174.04,       367.5,         10.08                      }, // aurous        quantum       x2.00    x2.50     x1.20
    {87804.57,       1102.5,        12.096                     }, // eternal       aurous        x1.75    x3.00     x1.20
    {158048.226,     2646,          13.3056                    }, // hyper         eternal       x1.80    x2.40     x1.10
    {237072.339,     7276.5,        15.30144                   }, // sunshine      hyper         x1.50    x2.75     x1.15
    {379315.7424,    21829.5,       17.596659                  }, // nebula        sunshine      x1.60    x3.00     x1.15
    {606905.1878,    43659,         21.1159872                 }, // infinity      nebula        x1.60    x2.00     x1.20
    {910357.7818,    109147.5,      23.22758592                }, // calamity      infinity      x1.50    x2.50     x1.10
    {1114956.024,    491163.75,     25.550344512               }, // unique        calamity      ^1.50    x4.50     x1.10
    {2787390.06,     1473491.25,    28.1053789632              }, // cosmic        unique        x2.50    x3.00     x1.10
    {83621701.8,     13261421.25,   35.131723704               }, // galactic      cosmic        x30.0    x9.00     x1.25
};
// clang-format on

uint32_t RR_RARITY_COLORS[rr_rarity_id_max] = {
    0xff7eef6d, 0xffffe65d, 0xff4d52e3, 0xff861fde,
    0xffde1f1f, 0xff1fdbde, 0xffff2b75, 0xffff00ff,
    0xfface3df, 0xffd1ab38, 0xff8d9ac9, 0xff467330,
    0xffc29c5b, 0xff4914a6, 0xff3d3d3d, 0xff852121,
    0xffeeeeee, 0xff692c9b, 0xffc0727d};

char const *RR_RARITY_NAMES[rr_rarity_id_max] = {
    "Common",    "Unusual", "Rare",     "Epic",
    "Legendary", "Mythic",  "Exotic",   "Ultimate",
    "Quantum",   "Auruos",  "Eternal",  "Hyper",
    "Sunshine",  "Nebula",  "Infinity", "Calamity",
    "Unique",    "Cosmic",  "Galactic"};

double RR_MOB_WAVE_RARITY_COEFFICIENTS[rr_rarity_id_max + 1] = {
    0,     1,     6,     10,
    15,    25,    160,   1200,
    2800,  4000,  9300,  12000,
    21000, 39000, 56000, 70000,
    106000};

double RR_DROP_RARITY_COEFFICIENTS[rr_rarity_id_unique + 2] = {
    0,     1,     8,     15,
    20,    145,   100,   105,
    30.5,  3.365, 5.75,  1.5,
    3.75,   45,    10,    5,
    20,    15};
double RR_MOB_LOOT_RARITY_COEFFICIENTS[rr_rarity_id_max] = {2.5,   4,     6,     15,
                                                            20,    40,    50,    55.5,
                                                            15,    2.675, 4.965, 2.675,
                                                            3.5,   25,    15,    10,
                                                            5,     20,    25};

static void init_game_coefficients()
{
    double sum = 1;
    double sum2 = 1;
    for (uint64_t a = 1; a < rr_rarity_id_max; ++a)
        RR_MOB_LOOT_RARITY_COEFFICIENTS[a] *=
            RR_MOB_LOOT_RARITY_COEFFICIENTS[a - 1];
    for (uint64_t a = 1; a <= rr_rarity_id_unique; ++a)
    {
        sum += (RR_DROP_RARITY_COEFFICIENTS[a + 1] =
                    RR_DROP_RARITY_COEFFICIENTS[a] /
                    RR_DROP_RARITY_COEFFICIENTS[a + 1]);
    }
    for (uint64_t a = 1; a <= rr_rarity_id_unique + 1; ++a)
    {
        RR_DROP_RARITY_COEFFICIENTS[a] = RR_DROP_RARITY_COEFFICIENTS[a] / sum +
                                         RR_DROP_RARITY_COEFFICIENTS[a - 1];
    }
    RR_DROP_RARITY_COEFFICIENTS[rr_rarity_id_unique + 1] = 1;



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
            maze_grid(x * 2, y * 2).difficulty = this_tile;
            maze_grid(x * 2 + 1, y * 2).difficulty = this_tile;
            maze_grid(x * 2, y * 2 + 1).difficulty = this_tile;
            maze_grid(x * 2 + 1, y * 2 + 1).difficulty = this_tile;
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
    uint32_t rarity_cap = rr_rarity_id_common + (difficulty + 18) / 19;
    if (rarity_cap > rr_rarity_id_galactic)
        rarity_cap = rr_rarity_id_galactic;
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

double RR_BASE_CRAFT_CHANCES[rr_rarity_id_max - 1] = {0.6000,  0.5500,  0.4500,  0.4000,
                                                      0.3000,  0.2000,  0.1000,  0.0900,
                                                      0.0800,  0.0700,  0.0600,  0.0500,
                                                      0.0400,  0.0300,  0.0200,  0.0100,
                                                      0.0030,  0.0025};
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
#define A 64

#define q 69
// old: 64
#define Q 72
// old: 68
#define z 76
// old: 72
#define Z 80
// old: 76
#define g 84
// old: 80
#define G 88
// old: 84
#define h 92
// old: 88
#define H 96
// old: 92
#define s 100
// old: 96
#define S 104
// old: 100
#define n 108
// old: 104
#define N 112
// old: 108
#define i 116
// old: 112
#define I 120
// old: 116
#define y 124
// old: 120
#define Y 128
// old: 124

#define RR_DEFINE_MAZE(name, size)                                             \
    struct rr_maze_grid RR_MAZE_##name[size][size];                            \
    uint8_t RR_MAZE_TEMPLATE_##name[size / 2][size / 2]
// clang-format off
RR_DEFINE_MAZE(HELL_CREEK, 160) = {
//                     11  13  15  17  19  21  23  25  27  29  31  33  35  37  39
// 1 2 3 4 5 6 7 8 9 10  12  14  16  18  20  22  24  26  28  30  32  34  36  38
{c,c,c,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,X,X,X,_,_,_,A,A,q,q,_,_,_,_,_,Q,Q,Q,_,_,_,_,_,g,g,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,_},
{c,c,c,c,c,_,_,_,_,_,_,_,_,_,x,_,_,_,_,_,_,_,_,_,a,X,X,X,X,X,X,X,X,_,_,A,A,A,q,q,q,q,q,Q,Q,Q,Q,Q,_,_,g,g,g,g,_,_,_,_,_,_,_,_,_,_,_,A,_,A,A,A,A,A,A,A,A,A,_,A},
{_,_,c,c,c,c,_,u,_,_,_,_,L,L,M,_,_,m,m,m,m,_,_,_,_,_,_,X,x,x,x,X,_,_,_,A,A,A,q,q,q,_,_,Q,Q,Q,Q,z,_,g,g,_,_,_,_,_,h,h,_,H,H,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,A,A},
{_,_,_,c,c,c,C,C,u,_,_,L,E,E,_,_,_,L,L,m,m,m,m,m,M,M,_,_,x,x,x,X,_,_,a,a,A,A,A,q,_,_,_,_,_,_,_,z,z,z,g,g,_,_,G,_,h,h,_,H,H,_,_,_,_,A,A,A,A,A,_,_,A,A,A,A,A,A},
{_,_,_,_,C,C,C,C,u,_,_,_,E,E,_,L,L,L,m,m,m,m,m,M,M,M,M,M,M,x,x,_,_,a,a,a,a,a,A,_,_,S,S,S,S,S,_,z,z,g,g,g,G,G,G,_,h,_,_,_,H,_,_,_,_,A,A,A,A,_,A,A,_,A,A,A,A,A},
{_,_,_,_,_,C,C,C,u,r,_,_,R,_,_,l,l,l,_,_,_,_,_,_,_,M,M,M,M,x,_,_,_,a,a,a,a,a,A,_,_,S,_,_,S,_,_,_,_,_,_,G,G,G,G,G,h,h,h,h,H,_,_,_,_,A,A,A,A,_,A,A,_,A,A,A,A,A},
{_,l,l,_,_,u,u,u,U,U,r,r,R,_,E,l,l,_,_,_,_,_,_,_,_,_,x,x,x,_,_,_,X,X,a,a,a,a,A,_,_,S,_,s,s,S,_,S,S,S,_,_,_,_,_,_,h,_,_,_,H,_,_,_,_,A,A,A,A,A,A,_,A,A,A,A,A,A},
{_,_,E,E,_,u,u,U,U,r,r,r,R,_,E,E,_,_,x,x,x,x,x,_,_,_,_,x,x,x,X,X,X,X,a,a,a,a,A,_,_,S,s,s,_,_,_,H,_,_,_,H,_,s,s,_,h,h,_,H,H,_,_,_,_,A,A,A,A,A,A,_,A,A,A,A,A,A},
{_,E,e,e,_,r,r,r,_,_,r,r,R,_,E,E,_,x,x,x,x,x,x,x,_,_,_,x,x,x,X,X,X,X,a,a,A,A,A,_,_,S,_,s,s,s,H,H,H,H,H,H,s,s,s,_,h,h,_,H,H,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,A,A},
{_,E,e,e,e,R,r,r,_,_,_,_,R,e,E,_,_,x,_,_,_,_,x,x,_,_,_,_,_,X,X,X,a,a,a,_,_,A,A,A,_,s,s,s,_,_,_,_,H,H,H,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,_,A,A,A,A,A,A},
{_,E,E,e,e,R,R,_,_,_,_,e,e,e,E,_,_,x,_,x,_,_,x,x,x,x,_,_,_,_,_,_,_,a,_,_,A,A,q,q,_,_,_,_,_,_,H,H,h,h,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,_,A,A,A,A,A,A,A,A,A,_,A},
{l,l,l,E,e,_,_,_,_,_,_,_,E,E,_,_,_,x,x,x,_,_,x,x,x,_,_,_,_,_,_,_,_,_,_,q,q,A,_,_,_,_,_,_,_,h,h,h,h,h,h,H,H,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,_},
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,_,_,A,A,_,_,_,_,z,z,_,_,_,_,_,_,_,q,q,q,q,_,_,_,_,_,_,_,_,h,h,h,H,H,_,_,_,_,x,x,x,x,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,A,_,_,_,z,z,z,z,Q,Q,_,_,q,q,q,q,_,_,_,_,G,G,G,G,G,h,h,h,H,H,_,_,_,_,_,_,x,x,x,x,_,_,_,x,x,x,x,x,_,_,x,x,x,_,_},
{_,_,_,_,_,_,_,_,_,_,_,_,A,A,_,A,_,_,A,A,A,A,A,A,A,_,_,z,z,z,z,Q,Q,Q,Q,q,q,_,_,_,_,_,G,G,G,G,G,G,h,h,h,H,_,_,_,_,x,x,_,_,x,x,x,x,_,_,_,_,x,x,x,_,_,x,x,x,_,_},
{_,_,_,_,_,_,A,_,_,_,_,A,A,A,A,A,A,_,_,A,A,A,_,_,A,_,_,z,Z,_,_,_,Q,Q,Q,Q,_,_,_,g,g,g,g,G,G,_,_,h,_,_,h,_,_,_,_,x,x,x,x,_,_,x,x,x,x,_,_,_,_,x,x,x,x,x,x,x,x,_},
{_,_,A,A,_,_,A,A,_,_,A,A,A,A,A,A,A,A,A,A,A,A,A,_,_,_,_,_,Z,Z,Z,_,_,_,Q,_,_,_,_,g,g,g,g,g,g,_,_,h,h,_,H,_,_,_,x,x,x,x,x,x,_,_,x,x,x,x,_,_,_,_,x,x,x,x,x,x,_,_},
{A,_,A,A,A,_,A,A,A,_,_,_,_,A,A,A,A,A,A,A,A,A,A,_,_,_,_,_,_,Z,Z,Z,_,_,_,_,g,g,g,g,g,g,g,G,_,_,_,h,h,_,_,_,_,x,x,x,x,_,_,x,x,x,x,x,x,x,x,_,x,_,x,x,x,x,x,x,_,_},
{A,_,_,A,A,_,A,A,A,A,A,_,_,A,A,A,A,A,_,_,_,_,_,_,_,_,_,_,_,Z,Z,Z,g,g,g,g,g,g,_,_,_,_,_,_,_,_,_,h,h,h,_,_,_,x,x,x,_,_,_,x,x,x,x,x,x,x,x,x,x,x,x,x,x,_,_,_,_,_},
{A,_,_,A,A,A,A,A,_,A,A,A,A,A,A,A,_,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,H,H,_,_,_,_,_,_,_,_,_,_,_,x,x,x,x,x,x,x,x,x,x,_,_,_,_,_,_,_},
{A,_,_,A,A,_,_,_,_,_,_,_,A,A,A,_,_,A,A,_,_,_,_,_,_,n,n,n,n,_,_,_,_,_,S,S,S,s,s,s,s,s,s,H,H,H,H,H,H,H,H,H,H,H,H,_,_,_,_,_,x,x,x,_,x,x,x,x,x,x,_,_,x,x,_,_,_,_},
{A,_,_,_,A,A,_,_,A,A,_,_,_,A,A,_,_,A,A,_,_,M,M,_,_,_,_,n,S,S,S,S,S,S,S,_,_,_,_,_,_,_,_,_,_,_,_,_,_,H,_,_,H,H,H,H,s,_,_,_,x,x,x,_,x,x,x,x,x,x,x,x,x,x,x,_,_,_},
{A,_,A,A,A,A,_,A,A,A,A,A,_,A,_,_,_,A,A,_,m,m,M,M,_,_,_,_,_,_,_,_,_,_,_,_,_,S,S,_,S,S,S,_,s,s,s,s,s,s,_,_,_,_,H,s,s,s,_,_,x,x,x,_,x,x,x,x,x,x,x,x,x,x,x,_,_,_},
{A,A,A,_,_,_,_,A,_,_,A,A,_,_,_,_,A,A,A,_,m,m,m,M,M,M,M,M,M,M,M,M,M,_,_,n,S,S,S,S,S,_,S,S,s,_,_,s,s,s,s,s,_,s,s,s,s,s,_,_,_,x,_,_,x,x,x,x,x,x,x,_,_,x,x,x,_,_},
{A,A,_,_,A,A,A,A,_,A,A,_,_,_,_,_,_,A,A,_,m,m,m,_,_,_,_,_,_,_,_,_,_,_,n,n,n,S,S,_,S,_,_,S,s,_,_,_,_,s,s,_,_,s,s,s,S,S,S,_,_,_,_,_,_,x,_,_,x,x,x,x,_,_,x,x,_,_},
{A,A,_,_,A,A,A,_,_,A,A,_,_,_,_,A,A,A,A,_,m,m,_,_,_,_,_,R,R,R,_,_,_,_,_,n,n,n,_,_,S,_,_,_,s,_,s,s,_,_,s,_,_,_,s,S,S,_,_,_,_,_,_,_,_,_,_,_,x,x,x,x,_,_,_,_,_,_},
{A,_,_,A,A,A,A,_,A,A,_,_,_,_,_,A,A,A,A,_,m,m,_,_,_,R,R,R,r,R,r,r,r,_,_,_,n,n,_,_,_,_,_,_,s,s,s,s,_,_,S,_,i,_,_,S,_,_,_,_,_,_,_,_,_,_,x,x,x,x,x,x,_,_,_,_,_,_},
{A,_,_,A,A,A,A,_,A,A,A,_,_,_,A,A,A,A,_,_,m,_,_,_,R,R,E,E,E,r,_,_,r,_,_,_,_,n,n,_,_,_,_,_,S,_,S,s,s,_,S,_,i,i,_,_,_,_,_,_,_,_,_,_,_,x,x,x,x,x,x,_,_,_,_,_,_,_},
{_,_,A,A,A,_,_,_,A,A,A,_,_,A,A,A,A,A,_,_,m,_,_,l,l,E,E,E,E,r,_,r,r,r,r,_,_,n,n,n,_,_,_,S,S,_,S,S,S,_,S,_,N,N,N,N,_,_,_,_,_,_,_,_,_,x,x,x,x,_,_,_,_,_,_,_,_,_},
{_,A,A,A,A,_,_,_,_,A,A,_,A,A,A,A,A,A,_,_,m,_,m,l,l,_,l,E,E,_,_,r,_,_,r,_,n,n,N,n,n,n,_,_,S,_,_,S,S,_,S,_,_,N,N,N,N,_,_,_,_,_,_,_,x,x,x,x,x,_,_,_,_,_,x,x,x,_},
{A,A,A,A,_,_,_,_,_,A,A,A,A,A,A,A,A,A,_,_,m,_,x,m,_,_,_,_,C,_,R,R,_,r,r,_,n,n,_,_,_,n,_,_,S,S,_,_,_,_,S,_,_,_,N,N,n,n,_,_,_,_,_,_,x,x,x,x,_,_,_,_,x,x,x,x,x,_},
{A,A,A,A,_,_,_,_,_,_,A,A,A,A,_,_,_,_,_,_,m,_,_,_,_,_,_,_,C,_,_,_,_,u,_,_,N,n,_,N,N,N,_,_,S,S,S,_,_,S,S,_,_,_,_,_,_,n,_,_,_,_,_,x,x,x,_,_,_,_,_,x,x,x,x,x,x,x},
{A,_,A,_,_,_,A,_,_,_,A,A,A,A,A,_,_,m,m,m,m,_,_,_,_,c,c,c,C,C,C,u,u,u,_,_,N,N,_,_,_,N,_,_,_,S,S,S,S,n,S,_,_,n,n,_,n,n,_,_,_,_,x,x,x,x,x,_,_,_,_,x,x,x,x,x,x,_},
{A,_,_,_,_,_,A,A,_,_,_,A,A,A,A,_,_,M,_,_,m,m,m,m,_,c,c,c,_,_,r,r,r,_,_,_,N,N,_,_,_,N,_,_,_,S,S,_,_,_,n,n,n,n,n,n,n,_,_,_,_,x,x,x,_,_,_,_,_,_,_,_,x,x,x,x,_,_},
{A,_,_,_,_,_,A,A,_,_,_,_,A,A,_,_,_,x,_,_,m,_,_,m,_,c,c,c,_,r,r,u,r,r,_,_,N,N,N,N,N,N,_,_,_,_,_,_,_,_,_,n,n,n,n,n,n,_,_,_,_,x,x,x,_,x,x,x,_,_,_,x,x,x,x,x,_,_},
{A,_,_,_,_,_,A,A,A,_,_,_,_,_,_,_,_,x,x,_,m,_,_,m,_,_,_,C,_,r,r,r,r,r,_,_,_,_,N,N,N,_,_,_,_,_,_,N,N,N,_,_,_,n,_,n,N,_,_,_,_,x,x,x,x,x,x,x,_,_,_,x,x,x,x,_,_,_},
{A,_,_,_,_,_,A,A,A,_,_,_,_,_,_,_,_,M,_,_,m,m,m,m,m,m,_,C,_,_,r,r,r,r,_,l,l,_,_,_,_,_,L,L,L,_,_,_,N,N,N,N,_,n,_,_,N,N,_,_,_,_,_,i,i,x,x,x,_,_,_,x,x,x,x,_,_,_},
{A,A,_,_,_,_,A,A,A,A,_,_,_,_,_,_,_,M,_,_,L,_,_,_,_,m,_,C,r,_,_,_,_,r,l,l,l,l,l,l,l,l,L,_,L,m,M,_,i,N,N,N,N,n,n,_,N,N,N,_,_,_,_,i,i,i,x,_,_,x,x,x,x,x,_,_,_,_},
{A,A,_,_,_,A,A,A,A,A,A,A,A,A,_,_,_,M,_,_,L,L,_,m,m,m,_,_,r,r,r,r,_,_,_,_,_,_,l,l,_,_,L,_,_,_,L,_,_,i,N,N,_,_,N,N,N,N,N,_,i,i,i,i,i,i,_,_,x,x,x,x,x,x,_,_,_,_},
{A,A,_,_,_,A,A,A,A,A,A,A,A,A,A,_,_,m,m,L,L,L,_,_,L,m,_,e,e,_,_,e,e,e,e,_,m,m,m,m,_,_,L,m,_,_,M,_,_,_,i,N,N,_,_,N,N,N,N,i,i,i,i,i,i,i,_,_,x,x,x,x,_,x,x,_,_,_},
{_,A,_,_,A,A,A,A,A,_,_,A,A,A,A,_,_,_,_,_,_,L,L,L,L,_,_,e,e,_,_,e,m,E,E,m,m,m,M,M,_,_,_,m,L,M,L,L,_,_,_,_,_,_,_,_,_,_,i,_,_,i,i,i,i,i,_,_,x,x,_,_,_,x,x,_,_,_},
{_,A,A,A,A,_,_,_,A,_,_,_,A,A,A,A,_,_,_,_,L,L,_,_,_,_,_,e,e,e,_,e,E,_,E,E,_,_,M,M,M,_,_,_,_,_,L,m,M,L,L,_,_,_,_,a,_,_,i,_,_,i,i,i,i,i,x,_,x,x,_,_,_,x,x,_,_,_},
{_,A,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,_,_,L,_,_,_,_,_,_,_,e,E,_,l,_,_,l,l,l,_,_,m,M,L,L,_,_,_,M,M,_,L,L,m,_,_,X,X,A,_,i,_,_,_,i,_,i,i,x,x,x,x,x,_,_,_,x,x,_,_},
{_,A,A,A,_,_,_,_,_,_,_,_,_,A,A,A,A,A,_,_,L,_,_,_,x,x,x,_,_,E,_,l,_,_,l,l,l,_,_,L,_,L,L,L,_,_,L,_,_,_,_,m,_,x,x,X,A,_,i,_,_,_,_,_,x,x,x,_,x,x,x,_,_,_,x,x,x,_},
{_,A,A,A,A,A,_,_,_,_,_,_,_,_,A,A,A,A,_,_,m,_,_,x,x,x,x,x,_,_,_,l,_,_,_,l,l,_,L,L,_,L,L,L,_,m,L,_,X,_,_,m,M,x,x,x,_,_,i,_,_,_,_,x,x,x,_,_,_,x,x,_,_,_,_,x,x,x},
{_,A,A,_,_,A,A,A,_,_,_,_,_,_,A,A,A,A,_,_,m,_,x,x,X,X,x,x,_,_,_,l,_,_,_,l,l,_,L,L,_,L,L,_,_,m,_,_,X,X,_,m,_,_,_,_,_,_,i,_,_,_,_,x,x,x,_,_,x,x,x,_,_,_,_,x,x,x},
{_,A,A,_,_,A,A,_,_,_,_,_,_,_,A,A,A,_,_,_,m,x,x,x,X,X,x,x,_,_,_,L,L,L,_,_,L,_,L,L,_,_,L,_,_,m,_,_,X,X,_,m,m,_,_,_,_,i,i,_,_,_,x,x,x,x,x,x,x,x,x,_,_,_,_,x,x,x},
{_,_,A,A,_,A,A,_,_,A,A,A,A,A,A,A,_,_,_,_,m,_,x,x,x,x,x,_,_,_,L,L,L,L,L,_,L,_,_,L,L,L,L,_,_,m,_,_,X,X,M,M,m,m,_,_,i,i,i,_,_,_,_,_,x,x,x,x,x,x,x,x,_,_,_,_,_,_},
{_,_,A,A,_,_,A,A,A,A,_,_,_,A,A,A,A,_,_,_,m,_,_,x,x,x,_,_,_,_,L,L,L,L,L,_,L,_,_,m,m,m,m,_,_,m,_,_,_,_,_,M,M,M,_,_,i,i,_,_,_,_,_,_,_,x,x,x,_,x,x,x,_,_,_,_,_,_},
{_,_,_,A,A,A,A,A,_,_,_,A,A,A,A,A,A,_,_,_,m,_,_,_,l,_,_,_,_,_,M,M,M,M,M,_,m,m,_,_,m,m,m,_,_,m,x,m,m,_,_,_,m,m,_,_,i,i,_,_,_,_,_,_,_,_,_,_,_,x,x,x,x,_,_,_,_,_},
{_,_,_,_,A,A,A,A,A,A,A,A,A,_,A,A,A,_,_,_,M,M,_,_,l,_,_,_,_,_,M,M,x,M,M,_,m,m,_,_,_,m,m,_,_,_,m,m,m,m,m,m,x,m,_,i,i,i,_,_,x,x,_,_,_,_,_,_,_,x,x,x,x,_,_,_,_,_},
{_,_,A,A,A,A,_,_,A,A,A,A,A,_,A,A,A,_,_,_,M,M,_,_,l,l,_,_,_,_,_,M,x,M,_,_,m,m,m,m,_,_,m,m,_,_,m,M,x,M,_,_,_,_,_,_,_,i,_,_,x,x,_,_,x,_,_,_,_,_,x,x,x,_,_,_,_,_},
{_,_,_,A,A,A,A,_,A,A,_,_,_,_,A,A,A,_,_,_,M,M,_,_,l,l,l,l,_,_,_,_,_,_,_,_,m,m,m,m,m,m,m,m,_,_,m,m,m,m,_,_,i,i,i,i,i,i,_,_,_,x,x,_,x,x,_,_,_,_,x,x,_,_,_,_,_,_},
{_,A,_,A,_,_,A,_,A,_,_,_,_,_,A,A,A,A,_,_,_,M,x,_,_,l,l,x,l,l,l,_,_,_,_,_,m,E,E,m,_,_,m,m,_,_,m,M,m,_,_,_,i,_,_,_,_,i,_,_,_,x,x,x,x,x,x,_,_,_,_,x,x,x,_,_,x,_},
{_,A,_,A,_,_,A,_,_,_,_,_,_,A,A,A,A,_,_,_,_,x,x,_,_,_,_,l,l,l,E,E,x,E,E,E,E,_,E,E,E,_,m,m,_,_,m,m,_,_,_,_,i,_,I,I,_,i,_,_,_,x,x,x,x,x,x,x,_,_,_,x,x,x,_,x,x,x},
{_,A,A,A,A,A,A,_,_,_,A,_,_,A,A,A,_,_,_,_,_,x,x,x,X,_,_,_,_,l,l,E,E,E,_,_,_,_,_,_,E,E,l,m,m,_,M,x,m,_,_,_,i,_,_,i,_,i,_,_,_,_,x,_,x,x,x,x,_,_,_,_,x,x,x,x,x,_},
{_,A,A,_,_,_,_,_,_,A,A,_,A,A,A,A,_,_,_,_,_,x,x,X,X,a,a,_,_,_,_,_,_,_,_,A,A,A,A,_,E,l,l,l,l,m,m,m,m,_,_,_,i,i,i,i,_,i,_,_,_,_,_,_,x,x,x,x,x,_,_,_,x,x,x,x,x,_},
{_,_,A,A,A,A,_,_,_,A,_,_,A,A,A,_,_,_,_,_,_,_,X,X,X,a,a,a,_,_,_,_,_,_,_,A,A,A,A,_,_,l,l,l,_,l,l,l,l,_,_,_,_,_,_,_,_,i,_,_,_,_,_,_,_,x,x,x,x,_,_,_,x,x,x,x,x,_},
{_,_,_,A,A,_,_,_,_,A,_,_,A,A,_,_,_,_,_,_,_,_,_,X,a,a,a,a,a,a,a,A,A,A,A,A,A,A,A,_,_,_,_,_,_,l,l,x,l,l,l,L,i,i,i,i,i,i,_,_,_,l,l,_,_,_,_,x,x,x,_,_,x,x,x,x,x,_},
{_,_,_,A,A,A,A,_,_,A,_,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,_,_,_,_,l,l,_,_,_,_,_,_,_,_,_,_,_,_,l,l,_,_,_,_,_,_,x,x,_,_,_,x,x,x,x,_},
{_,A,A,A,_,A,A,_,A,A,_,A,A,_,_,A,A,A,_,_,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,_,_,_,_,l,l,_,_,_,_,_,_,_,_,_,_,_,l,l,_,_,_,x,_,x,_,_,x,_,_,_,x,x,x,x,_},
{_,A,A,A,_,_,_,_,A,A,_,A,_,_,A,A,A,_,_,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,_,A,_,_,_,_,_,l,l,l,l,_,_,_,_,_,_,_,_,l,l,l,_,_,x,x,x,x,x,x,x,x,_,_,_,x,x,x,_,_},
{_,_,A,A,A,_,_,_,_,A,A,A,_,_,A,A,_,_,A,A,A,A,A,A,A,_,_,_,_,_,A,A,_,_,A,_,_,A,_,A,_,_,_,A,_,_,l,l,l,l,l,l,l,_,l,l,l,l,_,_,_,_,_,_,x,x,x,x,x,x,_,_,_,x,x,x,_,_},
{A,A,A,A,A,A,A,_,_,A,A,A,A,_,_,A,A,A,A,A,_,_,A,A,A,_,_,_,_,_,A,A,_,A,A,A,_,A,A,A,_,_,A,A,_,_,_,_,l,l,_,_,l,l,l,_,_,_,_,A,A,A,_,_,_,x,x,x,x,x,_,_,_,x,x,x,_,_},
{_,_,_,A,A,A,A,_,_,A,A,_,_,_,_,A,A,_,_,_,_,_,A,A,A,_,_,_,A,_,_,A,_,_,A,A,A,A,A,_,_,A,A,_,_,_,_,_,_,l,_,_,_,_,_,_,A,A,A,A,A,A,A,A,_,_,x,x,x,x,_,_,_,x,x,x,_,_},
{_,_,_,_,A,A,A,_,_,_,_,_,A,_,A,A,_,_,_,_,_,_,_,A,A,_,_,_,A,_,_,A,A,_,_,A,A,A,_,_,A,A,_,_,_,A,_,_,_,l,l,l,l,_,_,_,A,A,_,_,_,A,A,A,_,_,x,x,x,x,_,_,_,x,x,x,_,_},
{_,A,_,_,A,A,A,_,A,_,_,A,A,_,_,A,A,A,A,A,A,_,_,A,A,_,_,_,A,A,_,A,A,_,_,A,A,A,A,A,A,A,_,_,A,A,A,A,_,_,_,_,_,_,A,A,A,_,_,_,_,A,A,A,_,_,_,x,x,_,_,_,_,x,x,x,_,_},
{A,A,A,_,A,A,A,A,A,A,A,A,A,_,_,_,_,_,_,_,A,_,_,_,A,A,_,_,_,A,A,A,A,A,_,_,_,A,A,A,A,_,_,_,_,A,A,_,_,_,_,_,A,A,A,A,_,_,_,_,_,A,A,A,_,_,_,x,x,_,_,_,_,x,x,x,_,_},
{_,A,A,A,A,A,A,A,A,A,A,A,A,_,_,A,A,A,A,A,A,A,A,_,A,A,A,_,_,_,_,A,A,A,A,_,_,_,_,A,A,_,_,_,_,A,A,A,_,_,A,A,A,A,A,A,_,_,_,_,_,A,A,A,_,_,x,x,x,_,_,_,x,x,x,x,_,_},
{_,_,_,_,_,A,A,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,_,_,_,A,A,A,A,A,A,_,A,A,A,_,_,A,A,A,_,_,A,A,A,A,A,A,_,_,_,_,_,_,A,A,A,_,_,x,x,x,_,_,x,x,x,x,_,_,_},
{_,_,A,A,A,A,A,A,A,A,_,A,A,A,A,A,_,_,_,_,_,A,A,A,_,_,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,_,A,_,_,_,A,A,A,A,A,A,_,_,_,_,_,_,_,A,A,A,_,_,x,x,_,_,x,x,x,x,x,_,_,_},
{_,_,A,_,_,A,A,_,_,A,_,A,A,A,_,_,_,_,_,_,_,_,A,A,_,_,_,_,A,A,A,A,A,A,A,A,A,A,A,_,A,A,A,A,A,A,A,A,A,A,A,A,A,_,_,_,_,_,_,A,_,A,A,A,_,_,x,x,_,x,x,x,x,x,x,_,_,_},
{A,_,A,_,A,A,A,A,_,A,_,A,_,_,_,_,_,_,_,_,_,A,A,A,_,_,_,_,_,A,A,_,_,_,A,A,A,A,A,_,_,A,A,A,A,A,A,A,A,A,_,_,_,_,_,A,A,A,A,A,_,A,A,A,_,_,x,x,x,x,x,x,x,x,x,_,_,_},
{A,A,A,A,A,_,_,A,A,A,A,A,A,A,A,_,_,A,_,_,A,A,A,A,A,A,_,_,_,_,A,A,_,_,_,A,A,A,A,_,_,_,A,A,A,A,A,A,_,_,_,_,_,_,A,A,A,A,_,A,_,A,_,_,_,_,x,x,x,x,x,x,_,_,_,_,_,_},
{A,A,A,A,A,_,_,A,A,A,A,A,A,_,_,_,A,A,_,A,A,A,A,A,A,A,A,A,_,_,A,A,_,_,A,A,_,A,_,_,_,_,A,A,A,A,A,_,_,_,_,A,A,A,A,A,A,A,_,A,_,A,_,_,_,_,_,x,x,_,_,_,_,_,_,_,_,_},
{_,_,A,_,A,A,A,A,_,A,_,A,A,A,A,A,A,A,A,A,A,A,A,_,_,A,A,A,A,A,A,A,_,_,_,_,_,A,_,_,_,_,_,A,A,_,_,_,_,_,A,A,A,A,A,A,A,A,_,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
{_,_,A,_,_,A,A,_,_,A,_,_,A,A,_,A,A,A,_,_,A,A,_,_,_,_,A,A,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
{_,_,A,A,A,A,A,A,A,A,_,_,_,A,_,_,A,_,_,_,_,A,A,_,_,_,_,_,_,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,A,A,A,A,A,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}
/*
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 0
{_,A,A,A,A,A,_,_,X,X,X,X,x,x,x,x,x,x,x,x,x,x,x,M,M,M,M,M,M,M,M,M,M,M,M,M,M,x,x,_}, // 1
{_,A,A,A,_,_,_,X,X,X,X,_,_,x,x,x,_,_,_,x,_,x,x,_,_,_,_,_,_,M,_,_,M,M,M,x,_,_,x,_}, // 2
{_,A,A,a,a,_,_,X,X,X,_,_,X,X,x,_,_,x,x,x,x,x,_,_,M,M,M,M,_,M,M,_,_,_,x,x,x,_,_,_}, // 3
{_,A,_,a,a,a,_,_,X,_,_,X,X,X,_,_,x,x,x,x,_,x,_,M,M,M,_,M,M,M,M,M,_,x,x,x,x,x,_,_}, // 4
{_,A,_,_,a,a,a,_,_,_,X,X,X,_,_,X,x,x,_,x,x,x,_,M,M,_,_,_,_,M,M,_,_,x,x,x,x,x,_,_}, // 5
{_,_,_,_,_,a,a,_,_,X,X,X,_,_,X,X,X,_,_,x,_,x,_,M,M,_,M,M,_,_,m,_,_,_,x,x,x,_,_,_}, // 6
{_,_,_,_,a,a,_,_,X,X,X,_,_,X,X,X,_,_,x,x,x,x,_,M,M,M,M,M,M,_,m,_,m,_,_,x,_,_,m,_}, // 7
{_,_,_,a,a,_,_,a,X,X,_,_,_,_,X,X,X,_,_,_,x,x,_,M,M,_,M,M,M,_,m,_,m,m,_,_,_,m,m,_}, // 8
{_,_,X,X,_,_,a,a,a,_,_,x,_,_,X,X,X,X,X,_,x,x,_,_,M,_,_,M,M,_,m,_,m,m,m,m,_,_,L,_}, // 9
{_,X,X,_,_,a,a,a,_,_,x,x,_,X,X,_,_,_,X,_,x,X,X,_,M,M,_,_,_,_,m,_,_,m,m,m,m,_,L,_}, // 10
{_,X,X,_,a,a,a,_,_,x,x,x,_,X,X,_,a,X,X,_,X,X,X,_,_,m,m,m,m,m,m,m,m,m,m,m,L,L,L,_}, // 11
{_,X,X,_,_,a,a,_,x,x,x,x,_,X,X,_,_,_,X,_,X,_,_,_,_,_,m,m,m,m,_,_,_,_,_,_,L,L,L,_}, // 12
{_,_,X,X,_,_,_,_,x,x,_,x,_,X,X,_,a,_,a,_,X,_,A,A,a,_,_,_,_,_,_,A,A,A,A,_,_,_,L,_}, // 13
{_,_,_,X,X,_,_,x,x,_,_,M,_,_,X,a,a,a,a,_,X,_,_,a,a,a,a,a,a,a,a,a,A,A,A,A,_,_,L,_}, // 14
{_,m,_,_,X,X,x,x,_,_,M,M,_,_,_,a,a,a,_,_,X,X,_,_,a,_,_,a,_,_,_,_,A,A,A,A,A,_,L,_}, // 15
{_,m,m,_,_,X,X,_,_,M,M,M,_,a,_,_,_,_,_,X,X,X,X,_,_,_,a,a,_,_,E,_,_,_,_,_,_,_,L,_}, // 16
{_,m,m,m,_,_,_,_,_,_,_,M,_,a,a,a,a,X,X,X,X,X,X,X,a,a,a,_,_,E,E,l,_,_,l,l,l,_,L,_}, // 17
{_,m,m,m,_,M,M,M,_,M,M,M,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,E,E,_,l,l,_,l,_,l,_,L,_}, // 18
{_,m,_,m,m,m,_,M,M,M,M,M,_,e,e,e,e,e,e,e,e,_,E,E,E,E,E,E,E,_,_,_,l,l,l,_,l,L,L,_}, // 19
{_,m,_,_,_,_,_,_,_,_,_,_,_,e,e,e,e,e,_,_,e,E,E,_,_,_,_,_,E,E,_,l,l,_,_,_,_,_,_,_}, // 20
{_,m,m,m,L,L,L,_,l,l,l,l,_,_,_,_,e,e,e,_,_,_,_,_,R,R,R,_,_,E,E,l,_,_,U,U,U,U,U,_}, // 21
{_,_,_,_,_,_,L,_,l,l,l,l,l,l,l,_,_,e,e,e,e,e,R,R,R,R,R,R,_,_,E,_,_,_,U,U,_,_,U,_}, // 22
{_,L,L,_,L,L,L,_,_,_,_,l,l,_,l,l,_,_,_,e,_,_,_,_,_,R,R,R,R,_,_,_,r,_,_,r,r,_,U,_}, // 23
{_,_,L,_,_,L,_,_,l,l,_,_,l,l,l,l,l,_,e,e,e,_,c,c,_,_,R,R,R,R,R,R,r,r,_,r,r,_,U,_}, // 24
{_,_,L,L,L,L,_,l,l,l,l,_,l,_,l,_,l,_,e,e,e,_,c,c,c,_,_,_,_,_,_,_,r,r,_,_,r,_,U,_}, // 25
{_,l,L,_,_,_,_,l,l,l,l,_,l,l,l,l,l,_,_,e,_,_,_,c,_,_,C,_,C,C,u,_,_,r,r,r,r,_,U,_}, // 26
{_,l,_,_,l,l,_,_,_,l,_,_,l,_,l,l,_,_,e,e,_,_,_,C,C,_,C,C,C,_,u,u,_,_,_,_,_,_,U,_}, // 27
{_,l,_,_,l,l,l,l,l,l,l,l,l,l,l,_,_,e,e,_,_,M,_,_,C,C,C,_,C,u,u,u,u,u,u,u,u,u,u,_}, // 28
{_,l,l,_,_,_,_,_,_,l,_,E,l,l,_,_,E,e,_,_,M,M,x,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 29
{_,_,l,l,l,l,l,l,l,l,E,E,E,_,_,E,E,_,_,M,M,_,x,x,_,_,A,A,_,_,a,a,a,a,a,a,a,a,a,_}, // 30
{_,l,l,_,_,_,_,_,_,l,_,E,_,_,E,E,_,_,M,M,_,_,_,x,x,_,_,A,A,A,a,a,_,_,_,_,_,_,a,_}, // 31
{_,l,_,_,l,l,l,_,_,l,_,E,E,E,E,_,_,M,M,_,_,_,_,_,x,x,_,_,A,A,A,_,_,a,a,a,a,a,a,_}, // 32
{_,l,_,l,l,_,l,l,_,l,_,_,_,_,_,_,M,M,_,_,M,_,M,_,_,x,x,_,_,A,A,_,_,a,_,_,_,_,X,_}, // 33
{_,l,_,l,_,_,_,l,l,l,l,l,l,_,_,m,M,_,_,M,M,M,M,M,_,_,x,x,_,_,A,A,_,a,_,A,A,_,X,_}, // 34
{_,L,_,l,l,_,l,l,_,_,_,l,_,_,m,m,M,M,M,M,_,M,_,M,M,M,M,x,x,_,_,A,_,a,_,_,A,_,X,_}, // 35
{_,L,_,_,l,l,l,_,_,m,_,_,_,m,m,_,M,M,_,M,M,M,M,M,_,M,M,_,x,x,_,_,_,a,a,a,A,_,X,_}, // 36
{_,L,L,_,_,_,_,_,L,m,m,_,m,m,_,_,M,M,M,M,_,M,_,M,M,M,M,_,_,x,x,_,_,_,_,_,_,_,X,_}, // 37
{_,L,L,L,L,L,L,L,L,_,m,m,m,_,_,_,M,_,_,M,M,M,M,M,_,_,M,_,_,_,x,x,x,x,X,X,X,X,X,_}, // 38
{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}, // 39
*/
};
// clang-format on
RR_DEFINE_MAZE(BURROW, 4) = {{1, 1}, {0, 1}};

#define MAZE_ENTRY(MAZE, GRID_SIZE)                                            \
    (sizeof(RR_MAZE_##MAZE[0]) / sizeof(struct rr_maze_grid)), GRID_SIZE,      \
        &RR_MAZE_##MAZE[0][0]

struct rr_maze_declaration RR_MAZES[rr_biome_id_max] = {
    {MAZE_ENTRY(HELL_CREEK, 1024), 9, {
//       ??    ??    ??   ??   coordinate X   coordinate Y   level
        {22,   24,   3,   3,   1,             1,             1     }, // 0
        {34,   28,   3,   1,   35,            28,            20    }, // 1
        {34,   25,   3,   2,   35,            26,            40    }, // 2
        {18,   21,   3,   3,   19,            22,            60    }, // 3
        {26,   18,   3,   3,   27,            19,            80    }, // 4
        {9,    28,   4,   4,   11,            30,            80    }, // 5
        {38,   13,   1,   3,   38,            14,            100   }, // 6
        {1,    37,   2,   2,   1,             38,            100   }, // 7
        {4,    24,   2,   2,   5,             25,            100   }, // 8
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
#undef q
#undef Q
#undef z
#undef Z
#undef g
#undef G
#undef h
#undef H
#undef s
#undef S
#undef n
#undef N
#undef i
#undef I
#undef y
#undef Y
