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

#include <Server/EntityDetection.h>
#include <Server/Client.h>
#include <Server/Simulation.h>
#include <Server/SpatialHash.h>

#include <Shared/Vector.h>

#define MAX_ENTITY_CHOOSE_COUNT 256

struct entity_finder_captures
{
    struct rr_simulation *simulation;
    void *captures;
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *);
    EntityIdx closest;
    EntityIdx seeker;
    uint8_t seeker_team;
    float closest_dist;
    float x;
    float y;
};

struct entity_chooser_captures
{
    struct rr_simulation *simulation;
    void *captures;
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *);
    EntityIdx seeker;
    uint8_t seeker_team;
    float range;
    float x;
    float y;
    EntityIdx potential_vector[MAX_ENTITY_CHOOSE_COUNT];
    float dist_vector[MAX_ENTITY_CHOOSE_COUNT];
    uint32_t potential_count;
};

void shg_cb_enemy(EntityIdx potential, void *_captures)
{
    struct entity_finder_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    uint8_t allow =
        !rr_simulation_has_arena(simulation, potential) &&
        (rr_simulation_has_flower(simulation, potential) ||
         rr_simulation_has_mob(simulation, potential) ||
         (rr_simulation_has_petal(simulation, potential) &&
          (rr_simulation_get_petal(simulation, potential)->id == rr_petal_id_seed ||
           rr_simulation_get_petal(simulation, potential)->id == rr_petal_id_nest) &&
          rr_simulation_get_petal(simulation, potential)->detached));
    if (!allow)
        return;
    if (dev_cheat_enabled(simulation, potential, no_aggro))
        return;
    if (is_same_team(rr_simulation_get_relations(simulation, potential)->team,
                     captures->seeker_team))
        return;
    if (rr_simulation_get_health(simulation, potential)->health == 0)
        return;
    struct rr_component_physical *t_physical =
        rr_simulation_get_physical(simulation, potential);
    struct rr_vector delta = {captures->x - t_physical->x,
                              captures->y - t_physical->y};
    float dist =
        rr_vector_get_magnitude(&delta) * t_physical->aggro_range_multiplier -
        t_physical->radius;
    if (rr_simulation_has_petal(simulation, potential))
        dist *= 2;
    if (dist > captures->closest_dist)
        return;
    if (!captures->filter(simulation, captures->seeker, potential,
                          captures->captures))
        return;
    captures->closest_dist = dist;
    captures->closest = potential;
}

void shg_cb_friend(EntityIdx potential, void *_captures)
{
    struct entity_finder_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    if (!rr_simulation_has_mob(simulation, potential) &&
            !rr_simulation_has_flower(simulation, potential) ||
        rr_simulation_has_arena(simulation, potential))
        return;
    if (!is_same_team(rr_simulation_get_relations(simulation, potential)->team,
                      captures->seeker_team))
        return;
    if (rr_simulation_get_health(simulation, potential)->health == 0)
        return;
    struct rr_component_physical *t_physical =
        rr_simulation_get_physical(simulation, potential);
    struct rr_vector delta = {captures->x - t_physical->x,
                              captures->y - t_physical->y};
    float dist =
        rr_vector_get_magnitude(&delta) * t_physical->aggro_range_multiplier -
        t_physical->radius;
    if (dist > captures->closest_dist)
        return;
    if (!captures->filter(simulation, captures->seeker, potential,
                          captures->captures))
        return;
    captures->closest_dist = dist;
    captures->closest = potential;
}

void shg_cb_rand_enemy(EntityIdx potential, void *_captures)
{
    struct entity_chooser_captures *captures = _captures;
    struct rr_simulation *simulation = captures->simulation;
    uint8_t allow =
        !rr_simulation_has_arena(simulation, potential) &&
        (rr_simulation_has_flower(simulation, potential) ||
         rr_simulation_has_mob(simulation, potential) ||
         (rr_simulation_has_petal(simulation, potential) &&
          (rr_simulation_get_petal(simulation, potential)->id == rr_petal_id_seed ||
           rr_simulation_get_petal(simulation, potential)->id == rr_petal_id_nest) &&
          rr_simulation_get_petal(simulation, potential)->detached));
    if (!allow)
        return;
    if (dev_cheat_enabled(simulation, potential, no_aggro))
        return;
    if (is_same_team(rr_simulation_get_relations(simulation, potential)->team,
                     captures->seeker_team))
        return;
    if (rr_simulation_get_health(simulation, potential)->health == 0)
        return;
    struct rr_component_physical *t_physical =
        rr_simulation_get_physical(simulation, potential);
    struct rr_vector delta = {captures->x - t_physical->x,
                              captures->y - t_physical->y};
    float dist =
        rr_vector_get_magnitude(&delta) * t_physical->aggro_range_multiplier -
        t_physical->radius;
    if (rr_simulation_has_petal(simulation, potential))
        dist *= 2;
    if (dist > captures->range)
        return;
    if (!captures->filter(simulation, captures->seeker, potential,
                          captures->captures))
        return;
    if (captures->potential_count < MAX_ENTITY_CHOOSE_COUNT)
    {
        captures->potential_vector[captures->potential_count] = potential;
        captures->dist_vector[captures->potential_count] = dist;
        captures->potential_count += 1;
    }
    else
    {
        float farthest_dist = 0;
        uint32_t farthest_pos;
        for (uint32_t i = 0; i < captures->potential_count; ++i)
        {
            if (captures->dist_vector[i] > farthest_dist)
            {
                farthest_dist = captures->dist_vector[i];
                farthest_pos = i;
            }
        }
        if (dist < farthest_dist)
        {
            captures->potential_vector[farthest_pos] = potential;
            captures->dist_vector[farthest_pos] = dist;
        }
    }
}

EntityIdx rr_simulation_find_nearest_enemy(
    struct rr_simulation *simulation, EntityIdx seeker, float search_range,
    void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);

    return rr_simulation_find_nearest_enemy_custom_pos(
        simulation, seeker, physical->x, physical->y,
        search_range + physical->radius, captures, filter);
}

EntityIdx rr_simulation_find_nearest_enemy_custom_pos(
    struct rr_simulation *simulation, EntityIdx seeker, float x, float y,
    float min_dist, void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    EntityIdx target = RR_NULL_ENTITY;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, seeker);
    struct entity_finder_captures shg_captures;
    shg_captures.simulation = simulation;
    shg_captures.captures = captures;
    shg_captures.filter = filter;
    shg_captures.closest = target;
    shg_captures.seeker = seeker;
    shg_captures.closest_dist = min_dist;
    shg_captures.x = x;
    shg_captures.y = y;
    shg_captures.seeker_team = relations->team;
    struct rr_spatial_hash *shg =
        &rr_simulation_get_arena(simulation, physical->arena)->spatial_hash;
    rr_spatial_hash_query(shg, x, y, min_dist, min_dist, &shg_captures,
                          shg_cb_enemy);

    return shg_captures.closest;
}

EntityIdx rr_simulation_find_nearest_friend(
    struct rr_simulation *simulation, EntityIdx seeker, float search_range,
    void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);

    return rr_simulation_find_nearest_friend_custom_pos(
        simulation, seeker, physical->x, physical->y,
        search_range + physical->radius, captures, filter);
}

EntityIdx rr_simulation_find_nearest_friend_custom_pos(
    struct rr_simulation *simulation, EntityIdx seeker, float x, float y,
    float min_dist, void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    EntityIdx target = RR_NULL_ENTITY;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, seeker);
    struct entity_finder_captures shg_captures;
    shg_captures.simulation = simulation;
    shg_captures.captures = captures;
    shg_captures.filter = filter;
    shg_captures.closest = target;
    shg_captures.seeker = seeker;
    shg_captures.closest_dist = min_dist;
    shg_captures.x = x;
    shg_captures.y = y;
    shg_captures.seeker_team = relations->team;
    struct rr_spatial_hash *shg =
        &rr_simulation_get_arena(simulation, physical->arena)->spatial_hash;
    rr_spatial_hash_query(shg, x, y, min_dist, min_dist, &shg_captures,
                          shg_cb_friend);

    return shg_captures.closest;
}

EntityIdx rr_simulation_choose_nearby_enemy(
    struct rr_simulation *simulation, EntityIdx seeker, float search_range,
    void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);

    return rr_simulation_choose_nearby_enemy_custom_pos(
        simulation, seeker, physical->x, physical->y,
        search_range + physical->radius, captures, filter);
}

EntityIdx rr_simulation_choose_nearby_enemy_custom_pos(
    struct rr_simulation *simulation, EntityIdx seeker, float x, float y,
    float min_dist, void *captures,
    uint8_t (*filter)(struct rr_simulation *, EntityIdx, EntityIdx, void *))
{
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, seeker);
    struct entity_chooser_captures shg_captures;
    shg_captures.simulation = simulation;
    shg_captures.captures = captures;
    shg_captures.filter = filter;
    shg_captures.seeker = seeker;
    shg_captures.range = min_dist;
    shg_captures.x = x;
    shg_captures.y = y;
    shg_captures.seeker_team = relations->team;
    shg_captures.potential_count = 0;
    struct rr_spatial_hash *shg =
        &rr_simulation_get_arena(simulation, physical->arena)->spatial_hash;
    rr_spatial_hash_query(shg, x, y, min_dist, min_dist, &shg_captures,
                          shg_cb_rand_enemy);

    float sum = 0;
    for (uint32_t i = 0; i < shg_captures.potential_count; ++i)
        sum += 1 / shg_captures.dist_vector[i];
    float seed = rr_frand() * sum;
    for (uint32_t i = 0; i < shg_captures.potential_count; ++i)
        if ((seed -= 1 / shg_captures.dist_vector[i]) < 0)
            return shg_captures.potential_vector[i];
    return RR_NULL_ENTITY;
}

uint8_t no_filter(struct rr_simulation *simulation, EntityIdx seeker,
                  EntityIdx target, void *captures)
{
    return 1;
}

uint8_t high_zone_filter(struct rr_simulation *simulation, EntityIdx seeker,
                         EntityIdx target, void *captures)
{
    struct rr_component_mob *mob = rr_simulation_get_mob(simulation, seeker);
    if (mob->rarity < rr_rarity_id_ultimate)
        return 1;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, seeker);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    uint32_t grid_x = rr_fclamp(physical->x / arena->maze->grid_size,
                                0, arena->maze->maze_dim - 1);
    uint32_t grid_y = rr_fclamp(physical->y / arena->maze->grid_size,
                                0, arena->maze->maze_dim - 1);
    struct rr_maze_grid *grid =
        rr_component_arena_get_grid(arena, grid_x, grid_y);
    if (grid->difficulty < 48)
        return 1;
    physical = rr_simulation_get_physical(simulation, target);
    arena = rr_simulation_get_arena(simulation, physical->arena);
    grid_x = rr_fclamp(physical->x / arena->maze->grid_size,
                       0, arena->maze->maze_dim - 1);
    grid_y = rr_fclamp(physical->y / arena->maze->grid_size,
                       0, arena->maze->maze_dim - 1);
    grid = rr_component_arena_get_grid(arena, grid_x, grid_y);
    if (grid->difficulty >= 48 || grid->value == 0 || (grid->value & 8))
        return 1;
    return 0;
}
