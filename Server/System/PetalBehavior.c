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

#include <Server/System/System.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <Server/Client.h>
#include <Server/EntityAllocation.h>
#include <Server/EntityDetection.h>
#include <Server/Simulation.h>

#include <Shared/Entity.h>
#include <Shared/StaticData.h>
#include <Shared/Utilities.h>
#include <Shared/Vector.h>

struct area_captures
{
    struct rr_simulation *simulation;
    EntityIdx petal_id;
};

static void uranium_damage(EntityIdx target, void *_captures)
{
    struct area_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    if (!rr_simulation_has_mob(simulation, target) &&
        !rr_simulation_has_flower(simulation, target))
        return;
    if (is_dead_flower(simulation, target))
        return;
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, captures->petal_id);
    struct rr_component_relations *target_relations =
        rr_simulation_get_relations(simulation, target);
    if (is_same_team(relations->team, target_relations->team) &&
        relations->owner != rr_simulation_get_entity_hash(simulation, target))
        return;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, captures->petal_id);
    struct rr_component_physical *target_physical =
        rr_simulation_get_physical(simulation, target);
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, captures->petal_id);
    struct rr_vector delta = {physical->x - target_physical->x,
                              physical->y - target_physical->y};
    float radius = 400 * (petal->rarity + 1);
    if (rr_vector_magnitude_cmp(&delta, radius + target_physical->radius) == 1)
        return;
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, captures->petal_id);
    struct rr_component_health *target_health =
        rr_simulation_get_health(simulation, target);
    float damage = health->damage;
    if (relations->owner == rr_simulation_get_entity_hash(simulation, target))
        damage *= 3;
    target_health->flags |= 4;
    rr_component_health_do_damage(
        simulation, target_health, relations->owner, damage,
        rr_animation_color_type_uranium);
    if (!rr_simulation_has_ai(simulation, target))
        return;
    struct rr_component_ai *ai = rr_simulation_get_ai(simulation, target);
    struct rr_component_mob *mob = rr_simulation_get_mob(simulation, target);
    if (mob->player_spawned == 0 && mob->id != rr_mob_id_fern &&
        mob->id != rr_mob_id_tree && mob->id != rr_mob_id_meteor)
    {
        ai->ai_type = rr_ai_type_aggro;
        if (ai->aggro_range < radius + target_physical->radius)
            ai->aggro_range = radius + target_physical->radius;
    }
    if ((ai->target_entity == RR_NULL_ENTITY ||
         rr_frand() < powf(0.3, mob->rarity)) &&
        !dev_cheat_enabled(simulation, relations->owner, no_aggro))
        ai->target_entity = relations->owner;
}

static void uranium_petal_system(struct rr_simulation *simulation,
                                 struct rr_component_petal *petal)
{
    if (petal->effect_delay > 0)
        return;
    petal->effect_delay = RR_PETAL_DATA[petal->id].secondary_cooldown;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, petal->parent_id);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    float radius = 400 * (petal->rarity + 1);
    struct area_captures captures = {simulation, petal->parent_id};
    rr_spatial_hash_query(&arena->spatial_hash, physical->x, physical->y,
                          radius, radius, &captures, uranium_damage);
    struct rr_simulation_animation *animation =
        &simulation->animations[simulation->animation_length++];
    animation->type = rr_animation_type_area_damage;
    animation->owner = petal->parent_id;
    animation->x = physical->x;
    animation->y = physical->y;
    animation->size = radius;
    animation->color_type = rr_animation_color_type_uranium;
}

static void meat_aggro(EntityIdx target, void *_captures)
{
    // if (rr_frand() >= 0.015)
    //     return;
    struct area_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    if (!rr_simulation_has_mob(simulation, target))
        return;
    struct rr_component_mob *mob = rr_simulation_get_mob(simulation, target);
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, captures->petal_id);
    if (mob->rarity > petal->rarity)
        return;
    if (petal->aggro_count >= 10 + petal->rarity)
        return;
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, captures->petal_id);
    struct rr_component_relations *target_relations =
        rr_simulation_get_relations(simulation, target);
    if (is_same_team(relations->team, target_relations->team))
        return;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, captures->petal_id);
    struct rr_component_physical *target_physical =
        rr_simulation_get_physical(simulation, target);
    struct rr_vector delta = {physical->x - target_physical->x,
                              physical->y - target_physical->y};
    float radius = 300 + 100 * petal->rarity;
    if (rr_vector_magnitude_cmp(&delta, radius + target_physical->radius) == 1)
        return;
    struct rr_component_ai *ai = rr_simulation_get_ai(simulation, target);
    if (ai->target_entity != RR_NULL_ENTITY &&
        rr_simulation_has_petal(simulation, ai->target_entity) &&
        rr_simulation_get_petal(simulation,
                                ai->target_entity)->id == rr_petal_id_meat)
        return;
    if (dev_cheat_enabled(simulation, captures->petal_id, no_aggro))
        return;
    ai->target_entity =
        rr_simulation_get_entity_hash(simulation, captures->petal_id);
    ++petal->aggro_count;
}

static void meat_petal_system(struct rr_simulation *simulation,
                              struct rr_component_petal *petal)
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, petal->parent_id);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    float radius = 300 + 100 * petal->rarity;
    struct area_captures captures = {simulation, petal->parent_id};
    rr_spatial_hash_query(&arena->spatial_hash, physical->x, physical->y,
                          radius, radius, &captures, meat_aggro);
}

static void system_petal_detach(struct rr_simulation *simulation,
                                struct rr_component_petal *petal,
                                struct rr_component_player_info *player_info,
                                uint32_t outer_pos, uint32_t inner_pos,
                                struct rr_petal_data const *petal_data)
{
    rr_component_petal_set_detached(petal, 1);
    struct rr_component_player_info_petal *ppetal =
        &player_info->slots[outer_pos].petals[inner_pos];
    ppetal->entity_hash = RR_NULL_ENTITY;
    ppetal->cooldown_ticks = petal_data->cooldown;
}

static uint8_t is_close_enough_to_parent(struct rr_simulation *simulation,
                                         EntityIdx seeker, EntityIdx target,
                                         void *captures)
{
    struct rr_component_physical *parent_physical = captures;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, target);
    return ((physical->x - parent_physical->x) *
                    (physical->x - parent_physical->x) +
                (physical->y - parent_physical->y) *
                    (physical->y - parent_physical->y) <
            250 * 250);
}

static uint8_t is_mob_and_damaged(struct rr_simulation *simulation,
                                  EntityIdx seeker, EntityIdx target,
                                  void *captures)
{
    if (!rr_simulation_has_mob(simulation, target))
        return 0;
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, target);
    return health->max_health > health->health;
}

static uint8_t is_close_enough_and_angle(struct rr_simulation *simulation,
                                         EntityIdx seeker, EntityIdx target,
                                         void *captures)
{
    struct rr_component_physical *seeker_physical =
        rr_simulation_get_physical(simulation, seeker);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, target);
    struct rr_vector delta = {physical->x - seeker_physical->x,
                              physical->y - seeker_physical->y};
    if (rr_vector_magnitude_cmp(&delta, 750) == 1)
        return 0;
    return rr_angle_within(rr_vector_theta(&delta),
                           seeker_physical->bearing_angle, 1);
}

static void system_flower_petal_movement_logic(
    struct rr_simulation *simulation, EntityIdx id,
    struct rr_component_player_info *player_info, uint32_t rotation_pos,
    uint32_t outer_pos, uint32_t inner_pos,
    struct rr_petal_data const *petal_data)
{
    if (player_info->rotation_count == 0)
        return;
    struct rr_component_petal *petal = rr_simulation_get_petal(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    struct rr_vector position_vector = {physical->x, physical->y};
    struct rr_vector flower_vector = {flower_physical->x, flower_physical->y};
    float curr_angle = player_info->global_rotation +
                       rotation_pos * 2 * M_PI / player_info->rotation_count;

    if (petal->effect_delay == 0)
    {
        switch (petal->id)
        {
        case rr_petal_id_shell:
        {
            if ((player_info->input & 1) == 0)
                break;
            system_petal_detach(simulation, petal, player_info, outer_pos,
                                inner_pos, petal_data);
            petal->effect_delay = 75;
            physical->friction = 0.5;
            physical->bearing_angle = curr_angle;
            EntityIdx target = rr_simulation_find_nearest_enemy(
                simulation, id, 750, NULL, is_close_enough_and_angle);
            if (target != RR_NULL_ENTITY)
            {
                struct rr_component_physical *t_physical =
                    rr_simulation_get_physical(simulation, target);
                struct rr_vector delta = {t_physical->x - physical->x,
                                          t_physical->y - physical->y};
                physical->bearing_angle = rr_vector_theta(&delta);
            }
            break;
        }
        case rr_petal_id_peas:
        {
            if ((player_info->input & 1) == 0)
                break;
            system_petal_detach(simulation, petal, player_info, outer_pos,
                                inner_pos, petal_data);
            rr_vector_from_polar(&physical->acceleration, 4.0f,
                                 physical->angle);
            rr_vector_from_polar(&physical->velocity, 50.0f, physical->angle);
            petal->effect_delay = 38;
            uint32_t count = petal_data->count[petal->rarity];
            for (uint32_t i = 1; i < count; ++i)
            {
                EntityIdx new_petal = rr_simulation_alloc_petal(
                    simulation, physical->arena, physical->x, physical->y,
                    petal->id, petal->rarity, flower_physical->parent_id);
                struct rr_component_physical *new_physical =
                    rr_simulation_get_physical(simulation, new_petal);
                rr_component_physical_set_angle(
                    new_physical, physical->angle + i * 2 * M_PI / count);
                rr_vector_from_polar(&new_physical->acceleration, 4.0f,
                                     new_physical->angle);
                rr_vector_from_polar(&new_physical->velocity, 50.0f,
                                     new_physical->angle);
                rr_component_petal_set_detached(
                    rr_simulation_get_petal(simulation, new_petal), 1);
                rr_simulation_get_petal(simulation, new_petal)->effect_delay =
                    38;
            }
            break;
        }
        case rr_petal_id_azalea:
        {
            struct rr_component_health *flower_health =
                rr_simulation_get_health(simulation, player_info->flower_id);
            float heal = 9 * RR_PETAL_RARITY_SCALE[petal->rarity].heal /
                             petal_data->count[petal->rarity];
            if (flower_health->health < flower_health->max_health)
            {
                struct rr_vector delta = {
                    (flower_vector.x - position_vector.x),
                    (flower_vector.y - position_vector.y)};
                if (rr_vector_magnitude_cmp(&delta, flower_physical->radius +
                                                        physical->radius) == -1)
                {
                    float max_heal =
                        flower_health->max_health - flower_health->health;
                    rr_component_health_set_health(
                        flower_health, flower_health->health + heal);
                    rr_simulation_request_entity_deletion(simulation, id);
                    struct rr_simulation_animation *animation =
                        &simulation->animations[simulation->animation_length++];
                    animation->type = rr_animation_type_damagenumber;
                    animation->owner = player_info->flower_id;
                    animation->x = flower_physical->x;
                    animation->y = flower_physical->y;
                    if (max_heal < heal)
                        heal = max_heal;
                    animation->damage = ceilf(heal);
                    animation->color_type = rr_animation_color_type_heal;
                    return;
                }
                else
                {
                    rr_vector_scale(&delta, 0.4);
                    rr_vector_add(&physical->acceleration, &delta);
                    return;
                }
            }
            else
            {
                for (uint32_t i = 0; i < simulation->flower_count; ++i)
                {
                    EntityIdx potential = simulation->flower_vector[i];
                    if (is_dead_flower(simulation, potential))
                        continue;
                    struct rr_component_relations *target_relations =
                        rr_simulation_get_relations(simulation, potential);
                    if (!is_same_team(relations->team, target_relations->team))
                        continue;
                    struct rr_component_physical *target_physical =
                        rr_simulation_get_physical(simulation, potential);
                    struct rr_vector delta = {
                        (target_physical->x - position_vector.x),
                        (target_physical->y - position_vector.y)};
                    if (rr_vector_magnitude_cmp(&delta, 200) == 1)
                        continue;
                    flower_health =
                        rr_simulation_get_health(simulation, potential);
                    if (flower_health->health == flower_health->max_health)
                        continue;
                    if (rr_vector_magnitude_cmp(&delta,
                                                target_physical->radius +
                                                    physical->radius) == -1)
                    {
                        float max_heal =
                            flower_health->max_health - flower_health->health;
                        rr_component_health_set_health(
                            flower_health, flower_health->health + heal);
                        rr_simulation_request_entity_deletion(simulation, id);
                        struct rr_simulation_animation *animation =
                            &simulation->animations
                                 [simulation->animation_length++];
                        animation->type = rr_animation_type_damagenumber;
                        animation->owner = player_info->flower_id;
                        animation->x = target_physical->x;
                        animation->y = target_physical->y;
                        if (max_heal < heal)
                            heal = max_heal;
                        animation->damage = ceilf(heal);
                        animation->color_type = rr_animation_color_type_heal;
                        return;
                    }
                    else
                    {
                        rr_vector_scale(&delta, 0.4);
                        rr_vector_add(&physical->acceleration, &delta);
                        return;
                    }
                }
            }
            break;
        }
        case rr_petal_id_web:
        {
            if ((player_info->input & 3) == 0)
                break;
            system_petal_detach(simulation, petal, player_info, outer_pos,
                                inner_pos, petal_data);
            if (player_info->input & 1)
            {
                rr_vector_from_polar(&physical->acceleration, 7.5f, curr_angle);
                rr_vector_from_polar(&physical->velocity, 50.0f, curr_angle);
            }
            petal->effect_delay = 20;
            break;
        }
        case rr_petal_id_meat:
        {
            if ((player_info->input & 3) == 0)
                break;
            rr_component_petal_set_detached(petal, 1);
            if (player_info->input & 1)
            {
                rr_vector_from_polar(&physical->acceleration, 7.5f, curr_angle);
                rr_vector_from_polar(&physical->velocity, 50.0f, curr_angle);
            }
            petal->effect_delay = 500;
            break;
        }
        case rr_petal_id_nest:
        {
            if ((player_info->input & 2) == 0)
                break;
            petal->effect_delay = 15 * 25;
            rr_component_petal_set_detached(petal, 1);
            break;
        }
        case rr_petal_id_seed:
        {
            EntityIdx closest_target = RR_NULL_ENTITY;
            float closest_dist = 200;
            for (uint32_t i = 0; i < simulation->flower_count; ++i)
            {
                EntityIdx target = simulation->flower_vector[i];
                if (!is_dead_flower(simulation, target))
                    continue;
                struct rr_component_relations *target_relations =
                    rr_simulation_get_relations(simulation, target);
                if (!is_same_team(relations->team, target_relations->team))
                    continue;
                struct rr_component_physical *target_physical =
                    rr_simulation_get_physical(simulation, target);
                if (target_physical->bubbling_to_death)
                    continue;
                struct rr_vector delta = {target_physical->x - physical->x,
                                          target_physical->y - physical->y};
                if (rr_vector_magnitude_cmp(
                        &delta, closest_dist + target_physical->radius) == 1)
                    continue;
                closest_target = target;
                closest_dist =
                    rr_vector_get_magnitude(&delta) - target_physical->radius;
            }
            if (closest_target != RR_NULL_ENTITY)
            {
                struct rr_component_physical *target_physical =
                    rr_simulation_get_physical(simulation, closest_target);
                petal->bind_target =
                    rr_simulation_get_entity_hash(simulation, closest_target);
                rr_vector_from_polar(
                    &petal->bind_pos,
                    (target_physical->radius - physical->radius) * rr_frand(),
                    2 * M_PI * rr_frand());
                petal->effect_delay =
                    25 * RR_PETAL_RARITY_SCALE[petal->rarity].seed_cooldown;
                rr_component_petal_set_detached(petal, 1);
            }
            break;
        }
        case rr_petal_id_gravel:
        {
            if ((player_info->input & 3) == 0)
                break;
            system_petal_detach(simulation, petal, player_info, outer_pos,
                                inner_pos, petal_data);
            petal->effect_delay = 65;
            physical->friction = 0.4;
            break;
        }
        case rr_petal_id_mint:
        {
            EntityIdx mob_to_heal = rr_simulation_find_nearest_friend(
                simulation, id, 150, NULL, is_mob_and_damaged);
            if (mob_to_heal == RR_NULL_ENTITY)
                break;
            struct rr_component_physical *target_physical =
                rr_simulation_get_physical(simulation, mob_to_heal);
            struct rr_vector delta = {target_physical->x - physical->x,
                                      target_physical->y - physical->y};
            if (rr_vector_magnitude_cmp(&delta, target_physical->radius +
                                                    physical->radius) == -1)
            {
                struct rr_component_health *mob_health =
                    rr_simulation_get_health(simulation, mob_to_heal);
                float heal = 15 * RR_PETAL_RARITY_SCALE[petal->rarity].heal;
                float max_heal = mob_health->max_health - mob_health->health;
                rr_component_health_set_health(mob_health,
                                               mob_health->health + heal);
                rr_simulation_request_entity_deletion(simulation, id);
                struct rr_simulation_animation *animation =
                    &simulation->animations[simulation->animation_length++];
                animation->type = rr_animation_type_damagenumber;
                animation->owner = player_info->flower_id;
                animation->x = target_physical->x;
                animation->y = target_physical->y;
                if (max_heal < heal)
                    heal = max_heal;
                animation->damage = ceilf(heal);
                animation->color_type = rr_animation_color_type_heal;
                return;
            }
            else
            {
                rr_vector_scale(&delta, 0.4);
                rr_vector_add(&physical->acceleration, &delta);
                return;
            }
            break;
        }
        case rr_petal_id_bubble:
        {
            if ((player_info->input & 2) == 0)
                break;
            struct rr_vector accel = {player_info->client->player_accel_x,
                                      player_info->client->player_accel_y};
            if (accel.x || accel.y)
            {
                rr_simulation_request_entity_deletion(simulation, id);
                if (flower_physical->bubbling_to_death)
                    break;
                if (flower_physical->bubbling)
                    flower_physical->bubbling_to_death = 1;
                flower_physical->bubbling = 1;
                if (flower_physical->bubbling_to_death)
                {
                    rr_vector_set_magnitude(&accel, RR_PLAYER_SPEED * 100);
                    flower_physical->friction = 1;
                }
                else
                    rr_vector_set_magnitude(&accel, 25 * (petal->rarity + 1));
                rr_vector_add(&flower_physical->acceleration, &accel);
            }
            break;
        }
        default:
            break;
        }
    }
    else if (!petal->detached)
        --petal->effect_delay;
    else
        return;

    float holdingRadius = 50 + flower_physical->radius;
    uint8_t should_extend = player_info->input & 1 && !petal->no_rotation &&
                            petal_data->id != rr_petal_id_uranium &&
                            petal_data->id != rr_petal_id_magnet &&
                            petal_data->id != rr_petal_id_bone;
    if (petal->id == rr_petal_id_gravel && petal->detached)
        should_extend = player_info->input & 1;
    if (should_extend)
        holdingRadius = 125 + flower_physical->radius +
                            player_info->modifiers.petal_extension;
    else if (player_info->input & 2)
        holdingRadius = 20 + flower_physical->radius;
    struct rr_vector chase_vector;
    rr_vector_from_polar(&chase_vector, holdingRadius, curr_angle);
    rr_vector_add(&chase_vector, &flower_vector);
    /*
    EntityIdx gravity_target = rr_simulation_find_nearest_enemy(simulation, id,
    50, flower_physical, is_close_enough_to_parent); if (gravity_target !=
    RR_NULL_ENTITY)
    {
        struct rr_component_physical *t_phys =
    rr_simulation_get_physical(simulation, gravity_target); chase_vector.x +=
    (t_phys->x - chase_vector.x) * 0.8; chase_vector.y += (t_phys->y -
    chase_vector.y) * 0.8;
    }
    */
    rr_vector_sub(&chase_vector, &position_vector);
    if (petal_data->clump_radius != 0.0f &&
        petal_data->count[petal->rarity] != 1)
    {
        curr_angle = 1.333 * curr_angle +
                     2 * M_PI * inner_pos / petal_data->count[petal->rarity];
        struct rr_vector clump_vector;
        rr_vector_from_polar(&clump_vector, petal_data->clump_radius,
                             curr_angle);
        rr_vector_add(&chase_vector, &clump_vector);
    }
    if (petal->id == rr_petal_id_berry)
    {
        struct rr_vector random_vector;
        rr_vector_from_polar(&random_vector, 10.0f, rr_frand() * M_PI * 2);
        rr_vector_add(&chase_vector, &random_vector);
    }
    physical->acceleration.x += 0.5f * chase_vector.x;
    physical->acceleration.y += 0.5f * chase_vector.y;
    if (petal->id == rr_petal_id_fireball &&
        rr_vector_magnitude_cmp(&physical->acceleration, 1.0f) == 1)
        rr_component_physical_set_angle(
            physical, rr_vector_theta(&physical->acceleration));
    else
        rr_component_physical_set_angle(
            physical, physical->angle + 0.04f * petal->spin_ccw *
                                        (1 - petal->no_rotation));
}

static void petal_modifiers(struct rr_simulation *simulation,
                            struct rr_component_player_info *player_info)
{
    struct rr_component_flower *flower =
        rr_simulation_get_flower(simulation, player_info->flower_id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, player_info->flower_id);
    rr_component_flower_set_face_flags(flower, player_info->input);
    // reset
    physical->acceleration_scale = 1;
    player_info->modifiers.drop_pickup_radius = 25;
    player_info->modifiers.petal_extension = 0;
    player_info->modifiers.reload_speed = 1;

    physical->aggro_range_multiplier = 1;
    health->damage_reduction = 0;
    health->damage_reduction_ratio = 0;
    uint8_t rot_count = 0;
    float bone_diminish_factor = 1;
    float feather_diminish_factor = 1;
    float magnet_diminish_factor = 1;
    float third_eye_diminish_factor = 1;
    float to_rotate = 0.1;
    uint8_t crest_count = 0;
    uint8_t third_eye_count = 0;
    for (uint64_t outer = 0; outer < player_info->slot_count; ++outer)
    {
        struct rr_component_player_info_petal_slot *slot =
            &player_info->slots[outer];
        struct rr_petal_data const *data = &RR_PETAL_DATA[slot->id];
        if (data->id == rr_petal_id_leaf)
        {
            float heal = 0.075 * RR_PETAL_RARITY_SCALE[slot->rarity].heal;
            float max_heal = health->max_health - health->health;
            rr_component_health_set_health(health, health->health + heal);
            if (max_heal < heal)
                heal = max_heal;
            health->gradually_healed += heal;
        }
        else if (data->id == rr_petal_id_berry)
        {
            to_rotate += (0.02 + 0.012 * slot->rarity);
            player_info->modifiers.reload_speed += 0.02 * (slot->rarity + 1);
        }
        else if (data->id == rr_petal_id_feather)
        {
            physical->acceleration_scale +=
                (0.05 + 0.025 * slot->rarity) * feather_diminish_factor;
            feather_diminish_factor *= 0.5;
        }
        else if (data->id == rr_petal_id_crest)
        {
            ++crest_count;
            RR_SET_IF_LESS(player_info->camera_fov, 1 - 0.1 * slot->rarity)
        }
        else if (data->id == rr_petal_id_droplet)
            ++rot_count;
        else if (data->id == rr_petal_id_third_eye)
        {
            ++third_eye_count;
            player_info->modifiers.petal_extension +=
                45 * (slot->rarity - rr_rarity_id_epic) *
                    third_eye_diminish_factor;
            third_eye_diminish_factor *= 0.25;
        }
        else if (data->id == rr_petal_id_bone)
        {
            health->damage_reduction_ratio += 0.04 * (slot->rarity + 1) *
                                                  bone_diminish_factor;
            bone_diminish_factor *= 0.5;
        }
        else
        {
            for (uint32_t inner = 0; inner < slot->count; ++inner)
            {
                if (slot->petals[inner].entity_hash == RR_NULL_ENTITY)
                    continue;
                if (data->id == rr_petal_id_magnet)
                {
                    player_info->modifiers.drop_pickup_radius +=
                        (25 + 180 * slot->rarity) * magnet_diminish_factor;
                    magnet_diminish_factor *= 0.25;
                }
            }
        }
    }
    rr_component_flower_set_crest_count(flower, crest_count);
    rr_component_flower_set_third_eye_count(flower, third_eye_count);
    player_info->global_rotation +=
        to_rotate * ((rot_count % 3) ? (rot_count % 3 == 2) ? 0 : -1 : 1);
}

static void
system_egg_hatching_logic(struct rr_simulation *simulation,
                          struct rr_component_player_info *player_info,
                          struct rr_component_player_info_petal *p_petal)
{
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, p_petal->entity_hash);
    if (petal->effect_delay > 0)
        return;
    rr_simulation_request_entity_deletion(simulation, p_petal->entity_hash);
    uint8_t m_id, m_rar;
    if (petal->id == rr_petal_id_egg)
    {
        m_id = rr_mob_id_trex;
        m_rar = petal->rarity >= 1 ? petal->rarity - 1 : 0;
    }
    else if (petal->id == rr_petal_id_meteor)
    {
        m_id = rr_mob_id_meteor;
        m_rar = petal->rarity >= 1 ? petal->rarity - 1 : 0;
    }
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, p_petal->entity_hash);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, p_petal->entity_hash);
    EntityIdx mob_id = rr_simulation_alloc_mob(
        simulation, physical->arena, physical->x, physical->y, m_id, m_rar,
        relations->team);
    p_petal->entity_hash = rr_simulation_get_entity_hash(simulation, mob_id);
    struct rr_component_relations *mob_relations =
        rr_simulation_get_relations(simulation, mob_id);
    rr_component_relations_set_team(mob_relations, relations->team);
    rr_component_relations_set_owner(mob_relations, player_info->flower_id);
    rr_component_relations_update_root_owner(simulation, mob_relations);
    if (m_id == rr_mob_id_trex)
    {
        mob_relations->nest = relations->nest;
        rr_simulation_get_ai(simulation, mob_id)->ai_type = rr_ai_type_aggro;
    }
    rr_component_mob_set_player_spawned(
        rr_simulation_get_mob(simulation, mob_id), 1);
}

static void
system_nest_egg_choosing_logic(struct rr_simulation *simulation,
                               struct rr_component_player_info *player_info,
                               EntityHash id)
{
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    if (relations->nest != RR_NULL_ENTITY &&
        !rr_simulation_entity_alive(simulation, relations->nest))
        relations->nest = RR_NULL_ENTITY;
    if (relations->nest != RR_NULL_ENTITY)
    {
        struct rr_component_physical *nest_physical =
            rr_simulation_get_physical(simulation, relations->nest);
        struct rr_vector delta = {nest_physical->x - flower_physical->x,
                                  nest_physical->y - flower_physical->y};
        if (rr_vector_magnitude_cmp(&delta, 5000) == 1)
            relations->nest = RR_NULL_ENTITY;
    }
    if (relations->nest != RR_NULL_ENTITY)
        return;
    struct rr_component_relations *flower_relations =
        rr_simulation_get_relations(simulation, player_info->flower_id);
    if (flower_relations->nest != RR_NULL_ENTITY &&
        rr_simulation_entity_alive(simulation, flower_relations->nest))
    {
        relations->nest = flower_relations->nest;
        if (rr_frand() < 0.5)
            return;
    }
    EntityIdx nest_vector[RR_SQUAD_MEMBER_COUNT - 1];
    uint8_t nest_count = 0;
    for (uint32_t i = 0; i < simulation->nest_count; ++i)
    {
        EntityIdx nest_id = simulation->nest_vector[i];
        struct rr_component_relations *nest_relations =
            rr_simulation_get_relations(simulation, nest_id);
        if (nest_relations->owner == player_info->flower_id)
            continue;
        struct rr_component_player_info *p_info =
            rr_simulation_get_player_info(simulation, nest_relations->root_owner);
        if (p_info->squad != player_info->squad)
            continue;
        struct rr_component_physical *nest_physical =
            rr_simulation_get_physical(simulation, nest_id);
        struct rr_vector delta = {nest_physical->x - flower_physical->x,
                                  nest_physical->y - flower_physical->y};
        if (rr_vector_magnitude_cmp(&delta, 5000) == 1)
            continue;
        nest_vector[nest_count++] = nest_id;
    }
    if (nest_count > 0)
        relations->nest =
            rr_simulation_get_entity_hash(simulation,
                                          nest_vector[rand() % nest_count]);
}

static void system_nest_egg_movement_logic(struct rr_simulation *simulation,
                                           EntityHash id)
{
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    struct rr_component_nest *nest =
        rr_simulation_get_nest(simulation, relations->nest);
    nest->rotation_pos++;
    if (nest->rotation_count == 0)
        return;
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_physical *nest_physical =
        rr_simulation_get_physical(simulation, relations->nest);
    petal->effect_delay -= 2;
    if (petal->effect_delay < 0)
        petal->effect_delay = 0;
    struct rr_vector position_vector = {physical->x, physical->y};
    struct rr_vector nest_vector = {nest_physical->x, nest_physical->y};
    struct rr_vector chase_vector;
    float angle = 2 * M_PI * (nest->rotation_pos - 1) / nest->rotation_count +
                      nest->global_rotation;
    float radius = 100;
    rr_vector_from_polar(&chase_vector, radius, angle);
    rr_vector_add(&chase_vector, &nest_vector);
    rr_vector_sub(&chase_vector, &position_vector);
    rr_vector_scale(&chase_vector, 0.25);
    rr_vector_add(&physical->acceleration, &chase_vector);
}

static void rr_system_petal_reload_foreach_function(EntityIdx id,
                                                    void *simulation)
{
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(simulation, id);
    rr_component_player_info_set_camera_fov(
        player_info, RR_BASE_FOV / player_info->client->dev_cheats.fov_percent);
    if (!rr_simulation_entity_alive(simulation, player_info->flower_id) ||
        is_dead_flower(simulation, player_info->flower_id))
        return;
    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(simulation, player_info->flower_id);
    petal_modifiers(simulation, player_info);
    uint32_t rotation_pos = 0;
    uint8_t has_bubble = 0;
    for (uint64_t outer = 0; outer < player_info->slot_count; ++outer)
    {
        struct rr_component_player_info_petal_slot *slot =
            &player_info->slots[outer];
        struct rr_petal_data const *data = &RR_PETAL_DATA[slot->id];
        uint8_t max_cd = 0;
        uint8_t min_hp = 255;
        slot->count = slot->id == rr_petal_id_peas
                          ? 1
                          : RR_PETAL_DATA[slot->id].count[slot->rarity];
        uint8_t clump_count = data->clump_radius == 0 ? 1 : slot->count;
        for (uint64_t inner = 0; inner < slot->count; ++inner)
        {
            if (inner == 0 || data->clump_radius == 0)
                ++rotation_pos; // clump rotpos ++
            struct rr_component_player_info_petal *p_petal =
                &slot->petals[inner];
            if (p_petal->entity_hash != RR_NULL_ENTITY &&
                !rr_simulation_entity_alive(simulation, p_petal->entity_hash))
            {
                p_petal->entity_hash = RR_NULL_ENTITY;
                p_petal->cooldown_ticks = data->cooldown;
            }
            if (p_petal->entity_hash == RR_NULL_ENTITY)
            {
                if (slot->id == rr_petal_id_bubble && has_bubble)
                {
                    p_petal->cooldown_ticks = data->cooldown;
                    if (--clump_count == 0)
                        --rotation_pos;
                }
                float cd = rr_fclamp(
                    255.0f * p_petal->cooldown_ticks / data->cooldown, 0, 255);
                if (cd > max_cd)
                    max_cd = cd;
                p_petal->cooldown_ticks -= player_info->modifiers.reload_speed;
                if (p_petal->cooldown_ticks <= 0)
                {
                    p_petal->entity_hash = rr_simulation_get_entity_hash(
                        simulation,
                        rr_simulation_alloc_petal(
                            simulation, player_info->arena, flower_physical->x,
                            flower_physical->y, slot->id, slot->rarity,
                            player_info->flower_id));
                    struct rr_component_petal *petal =
                        rr_simulation_get_petal(simulation,
                                                p_petal->entity_hash);
                    petal->slot = slot;
                    petal->p_petal = p_petal;
                    if (data->id == rr_petal_id_meteor)
                        system_egg_hatching_logic(simulation, player_info,
                                                  p_petal);
                }
                if (data->id == rr_petal_id_meteor)
                {
                    if (--clump_count == 0)
                        --rotation_pos;
                }
            }
            else
            {
                struct rr_component_health *health = rr_simulation_get_health(
                    simulation, p_petal->entity_hash);
                float hp = rr_fclamp(
                    255.0f * health->health / health->max_health, 0, 255);
                if (hp < min_hp)
                    min_hp = hp;
                if (rr_simulation_get_physical(simulation, p_petal->entity_hash)
                        ->arena != player_info->arena ||
                    (slot->id == rr_petal_id_bubble && has_bubble))
                {
                    rr_simulation_request_entity_deletion(simulation,
                                                          p_petal->entity_hash);
                    continue;
                }
                if (data->id == rr_petal_id_egg)
                {
                    system_nest_egg_choosing_logic(simulation, player_info,
                                                   p_petal->entity_hash);
                    if (rr_simulation_has_petal(simulation, p_petal->entity_hash))
                    {
                        if (rr_simulation_get_relations(simulation,
                                p_petal->entity_hash)->nest != RR_NULL_ENTITY)
                            system_nest_egg_movement_logic(simulation,
                                                           p_petal->entity_hash);
                        system_egg_hatching_logic(simulation, player_info, p_petal);
                    }
                }
                if (!rr_simulation_has_petal(simulation, p_petal->entity_hash) ||
                    rr_simulation_get_relations(simulation,
                        p_petal->entity_hash)->nest != RR_NULL_ENTITY)
                {
                    if (--clump_count == 0)
                        --rotation_pos;
                    continue;
                }
                system_flower_petal_movement_logic(
                    simulation, p_petal->entity_hash, player_info,
                    rotation_pos - 1, outer, inner, data);
            }
        }
        // if (slot->id == rr_petal_id_bubble)
        //     has_bubble = 1;
        rr_component_player_info_set_slot_cd(player_info, outer, max_cd);
        rr_component_player_info_set_slot_hp(player_info, outer, min_hp);
    }
    player_info->rotation_count = rotation_pos;
}

static void system_petal_misc_logic(EntityIdx id, void *_simulation)
{
    struct rr_simulation *simulation = _simulation;
    struct rr_component_petal *petal = rr_simulation_get_petal(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    if (!rr_simulation_entity_alive(simulation, relations->owner) ||
        is_dead_flower(simulation, relations->owner))
    {
        rr_simulation_request_entity_deletion(simulation, id);
        return;
    }
    if (petal->detached == 0) // it's mob owned if this is true
    {
        if (petal->id == rr_petal_id_uranium)
            uranium_petal_system(simulation, petal);
        if (!rr_simulation_has_mob(simulation, relations->owner))
            return;
        if (--petal->effect_delay == 0)
            rr_simulation_request_entity_deletion(simulation, id);
        // check if owner is a mob
        return; // no logic yet
    }
    else
    {
        if (petal->id == rr_petal_id_shell)
        {
            rr_component_physical_set_angle(
                physical, physical->angle + 0.12f * (float)petal->spin_ccw);
            rr_vector_from_polar(&physical->acceleration, 15.0f,
                                 physical->bearing_angle);
        }
        else if (petal->id == rr_petal_id_peas)
            rr_vector_from_polar(&physical->acceleration, 7.5f,
                                 physical->angle);
        else if (petal->id == rr_petal_id_seed)
        {
            if (!rr_simulation_entity_alive(simulation, petal->bind_target) ||
                !is_dead_flower(simulation, petal->bind_target))
            {
                rr_simulation_request_entity_deletion(simulation, id);
                return;
            }
            struct rr_component_physical *target_physical =
                rr_simulation_get_physical(simulation, petal->bind_target);
            struct rr_vector delta = {target_physical->x - physical->x,
                                      target_physical->y - physical->y};
            rr_vector_add(&delta, &petal->bind_pos);
            rr_vector_scale(&delta, 0.4);
            rr_vector_add(&physical->acceleration, &delta);
            struct rr_component_relations *target_relations =
                rr_simulation_get_relations(simulation, petal->bind_target);
            struct rr_component_player_info *target_player_info =
                rr_simulation_get_player_info(simulation,
                                              target_relations->owner);
            rr_component_player_info_set_spectate_target(
                target_player_info, petal->bind_target);
            target_player_info->spectate_ticks = 62;
            target_player_info->spectating_single_target = 0;
        }
        else if (petal->id == rr_petal_id_meat)
            meat_petal_system(simulation, petal);
        if (--petal->effect_delay <= 0)
        {
            rr_simulation_request_entity_deletion(simulation, id);
            if (petal->id == rr_petal_id_seed)
            {
                struct rr_component_flower *target_flower =
                    rr_simulation_get_flower(simulation, petal->bind_target);
                rr_component_flower_set_dead(target_flower, simulation, 0);
            }
            else if (petal->id == rr_petal_id_nest)
            {
                struct rr_component_relations *flower_relations =
                    rr_simulation_get_relations(simulation, relations->owner);
                if (flower_relations->nest != RR_NULL_ENTITY &&
                    rr_simulation_entity_alive(simulation, flower_relations->nest))
                    rr_simulation_request_entity_deletion(
                        simulation, flower_relations->nest);
                EntityIdx nest_id = rr_simulation_alloc_entity(simulation);
                petal->p_petal->entity_hash = flower_relations->nest =
                    rr_simulation_get_entity_hash(simulation, nest_id);
                struct rr_component_nest *nest =
                    rr_simulation_add_nest(simulation, nest_id);
                nest->rarity = petal->rarity;
                struct rr_component_physical *nest_physical =
                    rr_simulation_add_physical(simulation, nest_id);
                rr_component_physical_set_x(nest_physical, physical->x);
                rr_component_physical_set_y(nest_physical, physical->y);
                rr_component_physical_set_radius(nest_physical, 250);
                rr_component_physical_set_angle(nest_physical, rr_frand() * 2 * M_PI);
                nest_physical->friction = 0.75;
                nest_physical->arena = physical->arena;
                struct rr_component_relations *nest_relations =
                    rr_simulation_add_relations(simulation, nest_id);
                rr_component_relations_set_team(nest_relations, relations->team);
                rr_component_relations_set_owner(nest_relations, relations->owner);
                rr_component_relations_update_root_owner(simulation, nest_relations);
                struct rr_component_health *nest_health =
                    rr_simulation_add_health(simulation, nest_id);
                uint8_t stats_rarity = nest->rarity > 0 ? nest->rarity - 1 : 0;
                rr_component_health_set_max_health(
                    nest_health, 150 * RR_MOB_RARITY_SCALING[stats_rarity].health);
                rr_component_health_set_health(nest_health, nest_health->max_health);
                nest_health->damage = 0;
                nest_health->damage_reduction =
                    5 * RR_MOB_RARITY_SCALING[stats_rarity].damage;
            }
        }
    }
}

static void system_nest_logic(EntityIdx id, void *_simulation)
{
    struct rr_simulation *simulation = _simulation;
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, id);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, id);
    struct rr_component_nest *nest = rr_simulation_get_nest(simulation, id);
    nest->global_rotation += 0.1;
    nest->rotation_count = nest->rotation_pos;
    nest->rotation_pos = 0;
    if (!rr_simulation_entity_alive(simulation, relations->owner) ||
        is_dead_flower(simulation, relations->owner))
        rr_simulation_request_entity_deletion(simulation, id);
    else
    {
        struct rr_component_physical *flower_physical =
            rr_simulation_get_physical(simulation, relations->owner);
        struct rr_vector delta = {physical->x - flower_physical->x,
                                  physical->y - flower_physical->y};
        if (rr_vector_magnitude_cmp(&delta, 5000) == 1)
            rr_simulation_request_entity_deletion(simulation, id);
    }
}

void rr_system_petal_behavior_tick(struct rr_simulation *simulation)
{
    rr_simulation_for_each_player_info(simulation, simulation,
                                       rr_system_petal_reload_foreach_function);
    rr_simulation_for_each_petal(simulation, simulation,
                                 system_petal_misc_logic);
    rr_simulation_for_each_nest(simulation, simulation,
                                system_nest_logic);
}
