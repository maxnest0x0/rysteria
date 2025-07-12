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

#include <stdio.h>
#include <string.h>

#include <Server/Simulation.h>
#include <Shared/Bitset.h>

static uint8_t should_entities_collide(struct rr_simulation *this, EntityIdx a,
                                       EntityIdx b)
{
#define exclude(component_a, component_b)                                      \
    if (rr_simulation_has_##component_a(this, a) &&                            \
        rr_simulation_has_##component_b(this, b))                              \
        return 0;                                                              \
    if (rr_simulation_has_##component_a(this, b) &&                            \
        rr_simulation_has_##component_b(this, a))                              \
        return 0;

    exclude(drop, petal);
    exclude(drop, flower);
    exclude(arena, petal);
    exclude(arena, mob);
    uint8_t team1 = rr_simulation_get_relations(this, a)->team;
    uint8_t team2 = rr_simulation_get_relations(this, b)->team;
    if (!is_same_team(team1, team2))
        return 1; // only drop doesn't care about team

#undef exclude

    return 1;
}

struct colliding_with_captures
{
    struct rr_simulation *simulation;
    struct rr_component_physical *physical;
};

static void web_logic(struct rr_simulation *this, EntityIdx entity1,
                      EntityIdx entity2)
{
    if (is_same_team(rr_simulation_get_relations(this, entity2)->team,
                     rr_simulation_get_relations(this, entity1)->team))
        return;
    if (!(rr_simulation_has_mob(this, entity2) ||
          rr_simulation_has_flower(this, entity2)))
        return;
    struct rr_component_web *web = rr_simulation_get_web(this, entity1);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity2);
    // physical->acceleration_scale *= rr_lerp(web->slow_factor, 1,
    // physical->slow_resist);
    float slow_factor = web->slow_factor;
    if (rr_simulation_has_flower(this, entity2))
        slow_factor = 0.2 + 0.8 * slow_factor;
    physical->web_slowdown *= slow_factor;
}

static void enter_arena(struct rr_simulation *this, EntityIdx arena,
                        EntityIdx enterer)
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, enterer);
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(
            this, rr_simulation_get_relations(this, enterer)->root_owner);
    physical->arena = arena;
    struct rr_component_arena *a = rr_simulation_get_arena(this, arena);
    struct rr_spawn_zone *respawn_zone = &a->respawn_zone;
    rr_component_physical_set_x(
        physical, respawn_zone->x + 2 * a->maze->grid_size * rr_frand());
    rr_component_physical_set_y(
        physical, respawn_zone->y + 2 * a->maze->grid_size * rr_frand());
    rr_vector_set(&physical->velocity, 0, 0);
    rr_vector_set(&physical->collision_velocity, 0, 0);
    a->first_squad_to_enter = player_info->squad;
    a->player_entered = 1;
}

// that's for the casting which is not impm
static void colliding_with_function(uint64_t i, void *_captures)
{
    struct colliding_with_captures *captures = _captures;
    struct rr_simulation *this = captures->simulation;
    struct rr_component_physical *physical1 = captures->physical;
    EntityIdx entity1 = physical1->parent_id;
    EntityIdx entity2 = i;
    struct rr_component_physical *physical2 =
        rr_simulation_get_physical(captures->simulation, entity2);

    if (!should_entities_collide(this, entity1, entity2))
        return;
    if (rr_simulation_has_arena(this, entity1) &&
        rr_simulation_has_flower(this, entity2))
    {
        enter_arena(this, entity1, entity2);
        return;
    }
    else if (rr_simulation_has_arena(this, entity2) &&
             rr_simulation_has_flower(this, entity1))
    {
        enter_arena(this, entity2, entity1);
        return;
    }

    if (rr_simulation_has_web(this, entity1))
        return web_logic(this, entity1, entity2);
    else if (rr_simulation_has_web(this, entity2))
        return web_logic(this, entity2, entity1);

    struct rr_vector position1 = {physical1->x, physical1->y};
    struct rr_vector position2 = {physical2->x, physical2->y};
    struct rr_vector delta = {physical2->x - physical1->x,
                              physical2->y - physical1->y};
    float distance = rr_vector_get_magnitude(&delta);
    if (distance == 0)
        return;
    struct rr_component_relations *relations1 =
        rr_simulation_get_relations(this, entity1);
    struct rr_component_relations *relations2 =
        rr_simulation_get_relations(this, entity2);
    float mass1 = physical1->mass;
    float mass2 = physical2->mass;
    uint8_t pushable_mob2 = rr_simulation_has_flower(this, entity1) &&
                            rr_simulation_has_mob(this, entity2) &&
                            is_same_team(relations1->team, relations2->team);
    uint8_t pushable_mob1 = rr_simulation_has_flower(this, entity2) &&
                            rr_simulation_has_mob(this, entity1) &&
                            is_same_team(relations1->team, relations2->team);
    uint8_t inf1 = rr_simulation_has_nest(this, entity1) ||
                   (pushable_mob2 &&
                    rr_simulation_get_mob(this, entity2)->player_spawned);
    uint8_t inf2 = rr_simulation_has_nest(this, entity2) ||
                   (pushable_mob1 &&
                    rr_simulation_get_mob(this, entity1)->player_spawned);
    if (pushable_mob2)
        mass2 = physical1->mass * 0.4f *
                    (rr_simulation_get_mob(this, entity2)->rarity + 1);
    if (pushable_mob1)
        mass1 = physical2->mass * 0.4f *
                    (rr_simulation_get_mob(this, entity1)->rarity + 1);
    float v2_coeff, v1_coeff;
    if (inf1 && inf2)
    {
        v2_coeff = 0.5f;
        v1_coeff = 0.5f;
    }
    else if (inf1)
    {
        v2_coeff = 1.0f;
        v1_coeff = 0.0f;
    }
    else if (inf2)
    {
        v2_coeff = 0.0f;
        v1_coeff = 1.0f;
    }
    else
    {
        v2_coeff = mass1 / (mass1 + mass2);
        v1_coeff = mass2 / (mass1 + mass2);
    }
    {
        float overlap = (distance - physical1->radius - physical2->radius);
        physical1->collision_velocity.x +=
            overlap * delta.x / distance * v1_coeff;
        physical1->collision_velocity.y +=
            overlap * delta.y / distance * v1_coeff;
        physical2->collision_velocity.x -=
            overlap * delta.x / distance * v2_coeff;
        physical2->collision_velocity.y -=
            overlap * delta.y / distance * v2_coeff;
    }

    {
/*
#define RESTITUTION 0.25f
rr_vector_normalize(&delta);
float vel_1 = (delta.x + physical1->velocity.x + delta.y +
physical1->velocity.y); float vel_2 = (delta.x + physical2->velocity.x + delta.y
+ physical2->velocity.y); if (vel_2 > vel_1) return; v1_coeff *= 2; v2_coeff *=
2; float vshared_coeff = (physical2->mass - physical1->mass) / (physical1->mass
+ physical2->mass); float vf_1 = -vshared_coeff * vel_1 + v1_coeff * vel_2;
float vf_2 = vshared_coeff * vel_2 + v2_coeff * vel_1;

physical1->collision_velocity.x += (vf_1 * RESTITUTION) * delta.x *
physical2->knockback_scale; physical1->collision_velocity.y += (vf_1 *
RESTITUTION) * delta.y * physical2->knockback_scale;
physical2->collision_velocity.x += (vf_2 * RESTITUTION) * delta.x *
physical1->knockback_scale; physical2->collision_velocity.y += (vf_2 *
RESTITUTION) * delta.y * physical1->knockback_scale;
//removing the parallel components
physical1->velocity.x -= delta.x * vel_1;
physical1->velocity.y -= delta.x * vel_1;
physical2->velocity.x -= delta.x * vel_2;
physical2->velocity.y -= delta.x * vel_2;
*/
// printf("%f %f\n", vf_1, vf_2);
#define KNOCKBACK_CONST (8.0f / 2)
        rr_vector_normalize(&delta);
        physical1->acceleration.x -=
            v1_coeff * KNOCKBACK_CONST * delta.x * physical2->knockback_scale;
        physical1->acceleration.y -=
            v1_coeff * KNOCKBACK_CONST * delta.y * physical2->knockback_scale;
        physical2->acceleration.x -= (v1_coeff - 1) * KNOCKBACK_CONST * delta.x *
                                     physical1->knockback_scale;
        physical2->acceleration.y -= (v1_coeff - 1) * KNOCKBACK_CONST * delta.y *
                                     physical1->knockback_scale;
#undef KNOCKBACK_CONST
    }
}

static void system_for_each_function(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;

    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);

    struct colliding_with_captures captures;
    captures.physical = physical;
    captures.simulation = this;

    for (uint64_t i = 0; i < physical->colliding_with_size; ++i)
        colliding_with_function(physical->colliding_with[i], &captures);
}

static void system_reset_collision_velocity(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;

    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    rr_vector_set(&physical->collision_velocity, 0, 0);
}

void rr_system_collision_resolution_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_physical(this, this,
                                    system_reset_collision_velocity);
    rr_simulation_for_each_physical(this, this, system_for_each_function);
}
