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
#include <string.h>

#include <Server/Client.h>
#include <Server/EntityDetection.h>
#include <Server/Simulation.h>
#include <Shared/Bitset.h>

struct colliding_with_captures
{
    struct rr_simulation *simulation;
    struct rr_component_health *health;
};

static void system_default_idle_heal(EntityIdx entity, void *captures)
{
    struct rr_simulation *this = captures;
    struct rr_component_health *health = rr_simulation_get_health(this, entity);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    if (is_dead_flower(this, entity))
        return;

    if (health->poison_ticks > 0)
    {
        --health->poison_ticks;
        rr_component_health_set_health(health, health->health - health->poison);
    }
    else
        health->poison = 0;
    if (health->gradually_healed > 0 && ++health->gradually_healed_ticks == 10)
    {
        struct rr_simulation_animation *animation =
            &this->animations[this->animation_length++];
        animation->type = rr_animation_type_damagenumber;
        animation->owner = entity;
        animation->x = physical->x;
        animation->y = physical->y;
        animation->damage = ceilf(health->gradually_healed);
        animation->color_type = rr_animation_color_type_heal;
        health->gradually_healed = 0;
        health->gradually_healed_ticks = 0;
    }
    if (health->damage_paused > 0)
        health->damage_paused -= 1;

    if (health->health == 0)
    {
        if (rr_simulation_has_flower(this, entity))
            rr_component_flower_set_dead(rr_simulation_get_flower(this, entity),
                                         this, 1);
        else
            rr_simulation_request_entity_deletion(this, entity);
    }
    else
    {
        float heal = (-0.0001 * log10f(health->max_health) + 0.0006) *
                         health->max_health;
        if (heal < 0)
            heal = health->max_health * 0.00001;
        rr_component_health_set_health(health, health->health + heal);
    }
}

struct lightning_captures
{
    EntityIdx *chain;
    uint32_t length;
};

static uint8_t lightning_filter(struct rr_simulation *simulation,
                                EntityIdx seeker, EntityIdx target,
                                void *captures)
{
    if (seeker == target)
        return 0;
    struct lightning_captures *chain = captures;
    for (uint32_t i = 0; i < chain->length; ++i)
    {
        if (chain->chain[i] == target)
            return 0;
    }
    return 1;
}

static void lightning_petal_system(struct rr_simulation *simulation,
                                   struct rr_component_petal *petal,
                                   EntityIdx first)
{
    struct rr_component_physical *petal_physical =
        rr_simulation_get_physical(simulation, petal->parent_id);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, petal->parent_id);
    struct rr_simulation_animation *animation =
        &simulation->animations[simulation->animation_length++];
    animation->type = rr_animation_type_lightningbolt;
    animation->owner = petal->parent_id;
    EntityIdx chain[16] = {petal->parent_id};
    animation->points[0].x = petal_physical->x;
    animation->points[0].y = petal_physical->y;
    uint32_t chain_amount = petal->rarity + 2;
    if (rr_simulation_has_petal(simulation, first))
        chain_amount = 1;
    float damage =
        rr_simulation_get_health(simulation, petal->parent_id)->damage;
    EntityIdx target = first;
    struct lightning_captures captures = {chain, 1};
    while (captures.length < chain_amount + 1)
    {
        if (target == RR_NULL_ENTITY)
            break;
        if (target != first && rr_simulation_has_ai(simulation, target))
        {
            struct rr_component_ai *ai =
                rr_simulation_get_ai(simulation, target);
            struct rr_component_mob *mob =
                rr_simulation_get_mob(simulation, target);
            if ((ai->target_entity == RR_NULL_ENTITY ||
                 rr_frand() < powf(0.3, mob->rarity)) &&
                !dev_cheat_enabled(simulation, relations->owner, no_aggro))
                ai->target_entity = relations->owner;
        }
        struct rr_component_physical *physical =
            rr_simulation_get_physical(simulation, target);
        struct rr_component_health *health =
            rr_simulation_get_health(simulation, target);
        health->flags |= 4;
        rr_component_health_do_damage(
            simulation, health, petal->parent_id, damage,
            rr_animation_color_type_lightning);
        health->damage_paused = 5;
        physical->stun_ticks = 4;
        chain[captures.length] = target;
        animation->points[captures.length].x = physical->x;
        animation->points[captures.length].y = physical->y;
        ++captures.length;
        target = rr_simulation_find_nearest_enemy_custom_pos(
            simulation, petal->parent_id, physical->x,
            physical->y, 400 + physical->radius,
            &captures, lightning_filter);
    }
    animation->length = captures.length;
    if (!dev_cheat_enabled(simulation, petal->parent_id, invulnerable))
        rr_simulation_request_entity_deletion(simulation, petal->parent_id);
}

struct fireball_captures
{
    struct rr_simulation *simulation;
    EntityIdx petal_id;
    EntityIdx exclude;
};

static void fireball_damage(EntityIdx target, void *_captures)
{
    struct fireball_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    if (target == captures->exclude)
        return;
    if (!rr_simulation_has_mob(simulation, target) &&
        !rr_simulation_has_petal(simulation, target) &&
        !rr_simulation_has_nest(simulation, target))
        return;
    if (is_dead_flower(simulation, target))
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
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, captures->petal_id);
    struct rr_vector delta = {physical->x - target_physical->x,
                              physical->y - target_physical->y};
    float radius = 50 * (petal->rarity + 1);
    if (rr_vector_magnitude_cmp(&delta, radius + target_physical->radius) == 1)
        return;
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, captures->petal_id);
    struct rr_component_health *target_health =
        rr_simulation_get_health(simulation, target);
    float damage = 0.2 * health->damage;
    target_health->flags |= 4;
    rr_component_health_do_damage(
        simulation, target_health, relations->owner, damage,
        rr_animation_color_type_fireball);
    if (!rr_simulation_has_ai(simulation, target))
        return;
    struct rr_component_ai *ai = rr_simulation_get_ai(simulation, target);
    struct rr_component_mob *mob = rr_simulation_get_mob(simulation, target);
    if ((ai->target_entity == RR_NULL_ENTITY ||
         rr_frand() < powf(0.3, mob->rarity)) &&
        !dev_cheat_enabled(simulation, relations->owner, no_aggro))
        ai->target_entity = relations->owner;
}

static void fireball_petal_system(struct rr_simulation *simulation,
                                  struct rr_component_petal *petal,
                                  EntityIdx exclude)
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, petal->parent_id);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    float radius = 50 * (petal->rarity + 1);
    struct fireball_captures captures = {simulation, petal->parent_id, exclude};
    rr_spatial_hash_query(&arena->spatial_hash, physical->x, physical->y,
                          radius, radius, &captures, fireball_damage);
    struct rr_simulation_animation *animation =
        &simulation->animations[simulation->animation_length++];
    animation->type = rr_animation_type_area_damage;
    animation->owner = petal->parent_id;
    animation->x = physical->x;
    animation->y = physical->y;
    animation->size = radius;
    animation->color_type = rr_animation_color_type_fireball;
    if (!dev_cheat_enabled(simulation, petal->parent_id, invulnerable))
        rr_simulation_request_entity_deletion(simulation, petal->parent_id);
}

static uint8_t damage_effect(struct rr_simulation *simulation, EntityIdx target,
                             EntityIdx attacker)
{
    if (rr_simulation_has_ai(simulation, target))
    {
        struct rr_component_ai *ai = rr_simulation_get_ai(simulation, target);
        struct rr_component_mob *mob =
            rr_simulation_get_mob(simulation, target);
        if (ai->target_entity == RR_NULL_ENTITY ||
            rr_frand() < powf(0.3, mob->rarity))
        {
            if (rr_simulation_has_petal(simulation, attacker) &&
                (rr_simulation_get_petal(simulation, attacker)->detached == 0 ||
                 (rr_simulation_get_petal(simulation, attacker)->id != rr_petal_id_seed &&
                  rr_simulation_get_petal(simulation, attacker)->id != rr_petal_id_nest &&
                  rr_simulation_get_petal(simulation, attacker)->id != rr_petal_id_meat)))
            {
                struct rr_component_relations *relations =
                    rr_simulation_get_relations(simulation, attacker);
                if (!dev_cheat_enabled(simulation, relations->owner, no_aggro))
                    ai->target_entity = relations->owner;
            }
            else if (!rr_simulation_has_nest(simulation, attacker) &&
                     !dev_cheat_enabled(simulation, attacker, no_aggro))
                ai->target_entity =
                    rr_simulation_get_entity_hash(simulation, attacker);
        }
    }
    if (rr_simulation_has_mob(simulation, attacker))
    {
        struct rr_component_mob *mob =
            rr_simulation_get_mob(simulation, attacker);
        if (mob->id == rr_mob_id_pachycephalosaurus &&
            mob->rarity >= rr_rarity_id_exotic)
        {
            struct rr_component_physical *physical =
                rr_simulation_get_physical(simulation, target);
            physical->stun_ticks = 25;
        }
    }
    else if (rr_simulation_has_petal(simulation, attacker))
    {
        struct rr_component_petal *petal =
            rr_simulation_get_petal(simulation, attacker);
        if (petal->id == rr_petal_id_shell)
        {
            struct rr_component_health *health =
                rr_simulation_get_health(simulation, attacker);
            struct rr_component_relations *relations =
                rr_simulation_get_relations(simulation, attacker);
            if (petal->detached &&
                rr_simulation_has_flower(simulation, relations->owner))
                health->damage =
                    sqrtf(1 + 0.4 * (75 - petal->effect_delay)) *
                        RR_PETAL_DATA[petal->id].damage *
                        RR_PETAL_DATA[petal->id].scale[petal->rarity].damage /
                        RR_PETAL_DATA[petal->id].count[petal->rarity];
        }
        else if (petal->id == rr_petal_id_beak)
        {
            struct rr_component_physical *physical =
                rr_simulation_get_physical(simulation, target);
            physical->stun_ticks =
                25 *
                (1 + sqrtf(RR_PETAL_RARITY_SCALE[petal->rarity].heal) / 3) *
                (1 - physical->slow_resist);
        }
        else if (petal->id == rr_petal_id_lightning)
        {
            lightning_petal_system(simulation, petal, target);
            return 0;
        }
        else if (petal->id == rr_petal_id_fireball)
            fireball_petal_system(simulation, petal, target);
        else if (petal->id == rr_petal_id_mandible)
        {
            struct rr_component_health *health =
                rr_simulation_get_health(simulation, target);
            if (health->health > 0 && health->health * 2 < health->max_health)
                rr_component_health_do_damage(
                    simulation, health, attacker,
                    50 * RR_PETAL_DATA[petal->id].scale[petal->rarity].damage,
                    rr_animation_color_type_damage);
        }
    }
    return 1;
}

static void colliding_with_function(uint64_t i, void *_captures)
{
    struct colliding_with_captures *captures = _captures;
    struct rr_simulation *this = captures->simulation;
    EntityIdx entity1 = captures->health->parent_id;
    EntityIdx entity2 = i;
    if (!rr_simulation_has_health(this, entity2))
        return;
    struct rr_component_relations *relations1 =
        rr_simulation_get_relations(this, entity1);
    struct rr_component_relations *relations2 =
        rr_simulation_get_relations(this, entity2);
    if (is_same_team(relations1->team, relations2->team))
        return;
    struct rr_component_health *health1 = captures->health;
    struct rr_component_health *health2 =
        rr_simulation_get_health(this, entity2);
    struct rr_component_physical *physical1 =
        rr_simulation_get_physical(this, entity1);
    struct rr_component_physical *physical2 =
        rr_simulation_get_physical(this, entity2);
    if (health2->health == 0)
        return;
    uint8_t bypass = rr_simulation_has_petal(this, entity1) ||
                     rr_simulation_has_petal(this, entity2);
    uint8_t byp2 = (rr_simulation_has_mob(this, entity1) &&
                    rr_simulation_has_mob(this, entity2));
    if (health1->damage_paused == 0 || bypass)
    {
        if (damage_effect(this, entity1, entity2))
        {
            rr_component_health_do_damage(
                this, health1, entity2, health2->damage,
                rr_animation_color_type_damage);
            health1->damage_paused = byp2 ? 3 : 8;
        }
    }
    if (health2->damage_paused == 0 || bypass)
    {
        if (damage_effect(this, entity2, entity1))
        {
            rr_component_health_do_damage(
                this, health2, entity1, health1->damage,
                rr_animation_color_type_damage);
            health2->damage_paused = byp2 ? 3 : 8;
        }
    }
}

static void system_for_each_function(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    // all health has physical

    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    struct rr_component_health *health = rr_simulation_get_health(this, entity);

    if (health->health == 0)
        return;

    struct colliding_with_captures captures;
    captures.health = health;
    captures.simulation = this;

    for (uint32_t i = 0; i < physical->colliding_with_size; ++i)
        colliding_with_function(physical->colliding_with[i], &captures);
}

void rr_system_health_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_health(this, this, system_default_idle_heal);
    rr_simulation_for_each_health(this, this, system_for_each_function);
}
