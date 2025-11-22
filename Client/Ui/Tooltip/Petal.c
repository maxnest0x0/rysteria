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

#include <Client/Ui/Ui.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <Client/Game.h>
#include <Client/Ui/Engine.h>

#include <Shared/StaticData.h>
#include <Shared/Utilities.h>

#define calculate_diminish_factor                                              \
    struct rr_ui_dynamic_text_metadata *data = this->data;                     \
    struct rr_ui_tooltip_metadata *tooltip_data = data->data;                  \
    uint8_t id = tooltip_data->id;                                             \
    uint8_t rarity = tooltip_data->rarity;                                     \
    char *extra = data->text;                                                  \
    float diminish_factor = 1;                                                 \
    if (tooltip_data->pos != 255)                                              \
    {                                                                          \
        if (game->simulation_ready)                                            \
        {                                                                      \
            for (uint8_t i = 0; i < tooltip_data->pos; ++i)                    \
                if (game->player_info->slots[i].id == id)                      \
                    diminish_factor *= 0.5;                                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            for (uint8_t i = 0; i < tooltip_data->pos; ++i)                    \
                if (game->cache.loadout[i].id == id)                           \
                    diminish_factor *= 0.5;                                    \
        }                                                                      \
    }

static void get_cooldown(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_ui_dynamic_text_metadata *data = this->data;
    struct rr_ui_tooltip_metadata *tooltip_data = data->data;
    uint8_t id = tooltip_data->id;
    uint8_t rarity = tooltip_data->rarity;
    char *cd = data->text;
    float reload_speed = 1;
    if (tooltip_data->pos != 255)
    {
        if (game->simulation_ready)
        {
            for (uint8_t i = 0; i < game->slots_unlocked; ++i)
                if (game->player_info->slots[i].id == rr_petal_id_berry)
                    reload_speed += 0.02 *
                                    (game->player_info->slots[i].rarity + 1);
        }
        else
        {
            for (uint8_t i = 0; i < game->slots_unlocked; ++i)
                if (game->cache.loadout[i].id == rr_petal_id_berry)
                    reload_speed += 0.02 * (game->cache.loadout[i].rarity + 1);
        }
    }
    if (RR_PETAL_DATA[id].cooldown == 0)
        cd[0] = 0;
    else if (id == rr_petal_id_seed)
        sprintf(cd, "↻ %.1f + %.1f + %.1fs",
                (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1 / reload_speed,
                (RR_PETAL_DATA[id].secondary_cooldown * 2 / 5) * 0.1,
                RR_PETAL_RARITY_SCALE[rarity].seed_cooldown);
    else if (id == rr_petal_id_nest)
        sprintf(cd, "↻ %.1f + %.1f + %.1fs",
                (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1 / reload_speed,
                (RR_PETAL_DATA[id].secondary_cooldown * 2 / 5) * 0.1, 15.0);
    else if (RR_PETAL_DATA[id].secondary_cooldown > 0)
        sprintf(cd, "↻ %.1f + %.1fs",
                (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1 / reload_speed,
                (RR_PETAL_DATA[id].secondary_cooldown * 2 / 5) * 0.1);
    else
        sprintf(cd, "↻ %.1fs",
                (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1 / reload_speed);
}

static void get_count(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_ui_dynamic_text_metadata *data = this->data;
    struct rr_ui_tooltip_metadata *tooltip_data = data->data;
    if (tooltip_data->count)
        sprintf(data->text, "x%u", tooltip_data->count);
    else
        data->text[0] = 0;
}

static void get_pickup_range(struct rr_ui_element *this, struct rr_game *game)
{
    calculate_diminish_factor
    sprintf(extra, "+%.0f", (25 + 180 * rarity) * diminish_factor);
}

static void get_speed_increase(struct rr_ui_element *this, struct rr_game *game)
{
    calculate_diminish_factor
    sprintf(extra, "%.1f%%", (5 + 2.5f * rarity) * diminish_factor);
}

static void get_damage_reduction(struct rr_ui_element *this,
                                 struct rr_game *game)
{
    calculate_diminish_factor
    sprintf(extra, "%.0f%%", 100 * 0.04 * (rarity + 1) * diminish_factor);
}

static void get_fov_increase(struct rr_ui_element *this, struct rr_game *game)
{
    calculate_diminish_factor
    sprintf(extra, "%.0f%%",
            (100 / (1 - 0.1 * rarity) - 100) * diminish_factor);
}

static void get_range_increase(struct rr_ui_element *this, struct rr_game *game)
{
    calculate_diminish_factor
    sprintf(extra, "+%.0f",
            45 * (rarity - rr_rarity_id_epic) * diminish_factor);
}

#undef calculate_diminish_factor

struct rr_ui_element *rr_ui_petal_tooltip_init(uint8_t id, uint8_t rarity)
{
    char fmt[16];
    char *hp = malloc((sizeof *hp) * 16);
    if (id != rr_petal_id_meteor)
        rr_sprintf(hp, RR_PETAL_DATA[id].health *
                           RR_PETAL_DATA[id].scale[rarity].health);
    else
        rr_sprintf(hp, RR_MOB_DATA[rr_mob_id_meteor].health *
                           RR_MOB_RARITY_SCALING[rarity >= 1 ? rarity - 1
                                                             : 0].health);
    char *dmg = malloc((sizeof *dmg) * 16);
    if (id != rr_petal_id_meteor)
        rr_sprintf(dmg, RR_PETAL_DATA[id].damage *
                            RR_PETAL_DATA[id].scale[rarity].damage /
                            RR_PETAL_DATA[id].count[rarity]);
    else
        rr_sprintf(dmg, RR_MOB_DATA[rr_mob_id_meteor].damage *
                            RR_MOB_RARITY_SCALING[rarity >= 1 ? rarity - 1
                                                              : 0].damage);

    struct rr_ui_tooltip_metadata *tooltip_data = malloc(sizeof *tooltip_data);
    tooltip_data->id = id;
    tooltip_data->rarity = rarity;
    struct rr_ui_element *cd = rr_ui_dynamic_text_init(16, 0xffffffff,
                                                       get_cooldown);
    struct rr_ui_dynamic_text_metadata *d_data = cd->data;
    d_data->data = tooltip_data;
    struct rr_ui_element *count = rr_ui_dynamic_text_init(16, 0xffffffff,
                                                          get_count);
    d_data = count->data;
    d_data->data = tooltip_data;
    struct rr_ui_element *this = rr_ui_set_background(
        rr_ui_v_container_init(
            rr_ui_tooltip_container_init(), 10, 5,
            rr_ui_flex_container_init(
                rr_ui_set_justify(
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 10,
                        rr_ui_text_init(RR_PETAL_NAMES[id], 24, 0xffffffff),
                        count,
                        NULL
                    ),
                -1, 0),
                rr_ui_set_justify(cd, 1, 0),
                30),
            rr_ui_set_justify(rr_ui_text_init(RR_RARITY_NAMES[rarity], 16,
                                              RR_RARITY_COLORS[rarity]),
                              -1, 0),
            rr_ui_static_space_init(10),
            rr_ui_set_justify(
                rr_ui_text_init(RR_PETAL_DESCRIPTIONS[id], 16, 0xffffffff), -1,
                0),
            NULL),
        0x80000000);
    struct rr_ui_container_metadata *data = this->data;
    data->data = tooltip_data;

    if (id != rr_petal_id_crest && id != rr_petal_id_third_eye &&
        id != rr_petal_id_lightning && id != rr_petal_id_fireball)
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Health: ", 12, 0xff44ff44),
                                  rr_ui_text_init(hp, 12, 0xffffffff), NULL),
                              -1, 0));
    if (id != rr_petal_id_shell && id != rr_petal_id_crest &&
        id != rr_petal_id_third_eye && id != rr_petal_id_meat)
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Damage: ", 12, 0xffff4444),
                                  rr_ui_text_init(dmg, 12, 0xffffffff), NULL),
                              -1, 0));
    if (id == rr_petal_id_shell)
    {
        char *extra = malloc((sizeof *extra) * 16);
        rr_sprintf(extra, (1 + 0.1 * 75) * RR_PETAL_DATA[id].damage *
                              RR_PETAL_DATA[id].scale[rarity].damage /
                              RR_PETAL_DATA[id].count[rarity]);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Damage: ", 12, 0xffff4444),
                          rr_ui_text_init(dmg, 12, 0xffffffff),
                          rr_ui_text_init(" ~ ", 12, 0xffffffff),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_magnet)
    {
        struct rr_ui_element *text =
            rr_ui_dynamic_text_init(12, 0xffffffff, get_pickup_range);
        d_data = text->data;
        d_data->data = tooltip_data;
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Pickup range: ", 12, 0xff44ffdd),
                          text, NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_leaf)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%.1f/s",
                25 * 0.075 * RR_PETAL_RARITY_SCALE[rarity].heal);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Heal: ", 12, 0xffffff44),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_egg)
    {
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Spawns: ", 12, 0xffe07422),
                          rr_ui_text_init(
                              RR_RARITY_NAMES[rarity >= 1 ? rarity - 1 : 0], 12,
                              RR_RARITY_COLORS[rarity >= 1 ? rarity - 1 : 0]),
                          rr_ui_text_init(" T-Rex", 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_berry)
    {
        char *extra = malloc((sizeof *extra) * 16);
        sprintf(extra, "%.1f rad/s", (0.02 + 0.012 * rarity) * 25);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Petal rotation: ", 12, 0xffd11b67),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
        extra = malloc((sizeof *extra) * 16);
        sprintf(extra, "+%.0f%%", 0.02 * (rarity + 1) * 100);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Petal reload speed: ", 12, 0xff12bef1),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_uranium)
    {
        char *extra = malloc((sizeof *extra) * 16);
        sprintf(extra, "%s", rr_sprintf(fmt, 400 * (rarity + 1)));
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Range: ", 12, 0xffbf29c2),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
        extra = malloc((sizeof *extra) * 16);
        rr_sprintf(extra, 3 * RR_PETAL_DATA[id].damage *
                              RR_PETAL_DATA[id].scale[rarity].damage);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Damage to owner: ", 12, 0xffff4444),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_feather)
    {
        struct rr_ui_element *text =
            rr_ui_dynamic_text_init(12, 0xffffffff, get_speed_increase);
        d_data = text->data;
        d_data->data = tooltip_data;
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Speed increase: ", 12, 0xff5682c4),
                          text, NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_azalea)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%s",
                rr_sprintf(fmt, 9 * RR_PETAL_RARITY_SCALE[rarity].heal));
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Heal: ", 12, 0xffffff44),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_bone)
    {
        struct rr_ui_element *text =
            rr_ui_dynamic_text_init(12, 0xffffffff, get_damage_reduction);
        d_data = text->data;
        d_data->data = tooltip_data;
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Damage reduction: ", 12, 0xffafafaf),
                          text, NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_web)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%.0f", RR_PETAL_RARITY_SCALE[rarity].web_radius);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Web radius: ", 12, 0xffafafaf),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
        extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%.0f%%", 100 * (1 - powf(0.56, rarity)));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Web slowdown: ", 12, 0xffe38329),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
        extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%.0f%%", 100 * (1 - powf(0.56, rarity)) * 0.8);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Web slowdown to flowers: ", 12, 0xffe38329),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_crest)
    {
        struct rr_ui_element *text =
            rr_ui_dynamic_text_init(12, 0xffffffff, get_fov_increase);
        d_data = text->data;
        d_data->data = tooltip_data;
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("FOV increase: ", 12, 0xffe38329),
                          text, NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_beak)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%.1fs",
                1 + sqrtf(RR_PETAL_RARITY_SCALE[rarity].heal) / 3);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Stun: ", 12, 0xff4266f5),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_lightning)
    {
        /*char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%s",
                rr_sprintf(fmt, RR_PETAL_DATA[id].damage *
                                    RR_PETAL_DATA[id].scale[rarity].damage /
                                    RR_PETAL_DATA[id].count[rarity] * 0.5));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Lightning: ", 12, 0xff00cfcf),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));*/
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%d", 2 + rarity);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Bounces: ", 12, 0xfffc00cf),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_third_eye)
    {
        struct rr_ui_element *text =
            rr_ui_dynamic_text_init(12, 0xffffffff, get_range_increase);
        d_data = text->data;
        d_data->data = tooltip_data;
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Range increase: ", 12, 0xff4266f5),
                          text, NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_nest)
    {
        uint8_t stats_rarity = rarity > 0 ? rarity - 1 : 0;
        char *extra = malloc((sizeof *extra) * 8);
        rr_sprintf(extra, 150 * RR_MOB_RARITY_SCALING[stats_rarity].health);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Nest health: ", 12, 0xff44ff44),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
        extra = malloc((sizeof *extra) * 8);
        rr_sprintf(extra, 5 * RR_MOB_RARITY_SCALING[stats_rarity].damage);
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Nest damage reduction: ", 12, 0xff666666),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Egg reload speed: ", 12, 0xff12bef1),
                          rr_ui_text_init("x2", 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_fireball)
    {
        char *extra = malloc((sizeof *extra) * 8);
        rr_sprintf(extra, 50 * (rarity + 1));
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Range: ", 12, 0xffbf29c2),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
        extra = malloc((sizeof *extra) * 8);
        rr_sprintf(extra, 0.2 * RR_PETAL_DATA[id].damage *
                              RR_PETAL_DATA[id].scale[rarity].damage);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Area damage: ", 12, 0xffff4444),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    else if (id == rr_petal_id_meat)
    {
        char *extra = malloc((sizeof *extra) * 8);
        rr_sprintf(extra, 300 + 100 * rarity);
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Range: ", 12, 0xffbf29c2),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Max mob rarity: ", 12, 0xffe07422),
                          rr_ui_text_init(
                              RR_RARITY_NAMES[rarity], 12,
                              RR_RARITY_COLORS[rarity]), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_bubble)
    {
        char *extra = malloc((sizeof *extra) * 16);
        sprintf(extra, "%.0f", 25.0f * (rarity + 1));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Boost: ", 12, 0xff5682c4),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_meteor)
    {
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Spawns: ", 12, 0xffe07422),
                          rr_ui_text_init(
                              RR_RARITY_NAMES[rarity >= 1 ? rarity - 1 : 0], 12,
                              RR_RARITY_COLORS[rarity >= 1 ? rarity - 1 : 0]),
                          rr_ui_text_init(" Meteor", 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_mandible)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%s",
                rr_sprintf(fmt, 10 * RR_PETAL_DATA[id].damage *
                                    RR_PETAL_DATA[id].scale[rarity].damage));
        rr_ui_container_add_element(
            this, rr_ui_set_justify(
                      rr_ui_h_container_init(
                          rr_ui_container_init(), 0, 0,
                          rr_ui_text_init("Extra Damage: ", 12, 0xff12bef1),
                          rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                      -1, 0));
    }
    else if (id == rr_petal_id_mint)
    {
        char *extra = malloc((sizeof *extra) * 8);
        sprintf(extra, "%s",
                rr_sprintf(fmt, 15 * RR_PETAL_RARITY_SCALE[rarity].heal));
        rr_ui_container_add_element(
            this,
            rr_ui_set_justify(rr_ui_h_container_init(
                                  rr_ui_container_init(), 0, 0,
                                  rr_ui_text_init("Heal: ", 12, 0xffffff44),
                                  rr_ui_text_init(extra, 12, 0xffffffff), NULL),
                              -1, 0));
    }
    return this;
}