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

#include <Server/Simulation.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <Server/Client.h>
#include <Server/EntityAllocation.h>
#include <Server/EntityDetection.h>
#include <Server/MobAi/Ai.h>
#include <Server/SpatialHash.h>
#include <Server/System/System.h>
#include <Server/Waves.h>
#include <Shared/Bitset.h>
#include <Shared/Crypto.h>
#include <Shared/Utilities.h>
#include <Shared/pb.h>

static void set_respawn_zone(struct rr_component_arena *arena, uint32_t x,
                             uint32_t y)
{
    float dim = arena->maze->grid_size;
    arena->respawn_zone.x = 2 * x * dim;
    arena->respawn_zone.y = 2 * y * dim;
}

#define SPAWN_ZONE_X 1
#define SPAWN_ZONE_Y 1

static void set_special_zone(uint8_t biome, uint8_t (*fun)(), uint32_t x,
                             uint32_t y, uint32_t w, uint32_t h)
{
    x *= 2;
    y *= 2;
    w *= 2;
    h *= 2;
    uint32_t dim = RR_MAZES[biome].maze_dim;
    for (uint32_t Y = 0; Y < h; ++Y)
        for (uint32_t X = 0; X < w; ++X)
            RR_MAZES[biome].maze[(Y + y) * dim + (X + x)].spawn_function = fun;
}

#define ALL_MOBS 255
#define DIFFICULT_MOBS 254

uint8_t fern_zone() { return rr_mob_id_fern; }
uint8_t pter_meteor_zone()
{
    return rr_frand() > 0.02 ? rr_mob_id_pteranodon : rr_mob_id_meteor;
}
uint8_t ornith_pachy_zone()
{
    return rr_frand() > 0.5 ? rr_mob_id_ornithomimus : rr_mob_id_pachycephalosaurus;
}
uint8_t trice_dako_zone()
{
    return rr_frand() > 0.2 ? rr_mob_id_dakotaraptor : rr_mob_id_triceratops;
}
uint8_t anky_trex_zone()
{
    return rr_frand() > 0.2 ? rr_mob_id_ankylosaurus : rr_mob_id_trex;
}
uint8_t edmo_zone() { return rr_mob_id_edmontosaurus; }
// ~x5 tree chance
uint8_t tree_zone() {
    return rr_frand() > 0.0025 ? DIFFICULT_MOBS : rr_mob_id_tree;
}
uint8_t pter_zone() {
    return rr_frand() > 0.2 ? rr_mob_id_pteranodon : ALL_MOBS;
}

struct zone
{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint8_t (*spawn_func)();
};

#define ZONE_POSITION_COUNT 9

static struct zone zone_positions[ZONE_POSITION_COUNT] = {
    {4,  0,  7,  3, fern_zone},
    {16, 3,  5,  4, pter_meteor_zone},
    {17, 8,  3,  2, ornith_pachy_zone},
    {26, 11, 3,  3, trice_dako_zone},
    {10, 22, 4,  3, anky_trex_zone},
    {23, 22, 4,  3, edmo_zone},
    {16, 25, 5,  2, tree_zone},
    {13, 27, 11, 4, tree_zone},
    {2,  13, 6, 17, pter_zone},
};

static void set_spawn_zones()
{
    for (uint64_t i = 0; i < ZONE_POSITION_COUNT; i++)
    {
        struct zone zone = zone_positions[i];
        set_special_zone(rr_biome_id_hell_creek, zone.spawn_func, zone.x, zone.y,
                         zone.w, zone.h);
    }
}

void rr_simulation_init(struct rr_simulation *this)
{
    memset(this, 0, sizeof *this);
    EntityIdx id = rr_simulation_alloc_entity(this);
    struct rr_component_arena *arena = rr_simulation_add_arena(this, id);
    arena->biome = RR_GLOBAL_BIOME;
    rr_component_arena_spatial_hash_init(arena, this);
    set_respawn_zone(arena, SPAWN_ZONE_X, SPAWN_ZONE_Y);
    set_spawn_zones();
}

struct too_close_captures
{
    struct rr_simulation *simulation;
    float x;
    float y;
    float closest_dist;
    uint8_t done;
};

static void too_close_cb(EntityIdx potential, void *_captures)
{
    struct too_close_captures *captures = _captures;
    if (captures->done)
        return;
    struct rr_simulation *simulation = captures->simulation;
    if (!rr_simulation_has_mob(simulation, potential) &&
            !rr_simulation_has_flower(simulation, potential) ||
        rr_simulation_has_arena(simulation, potential))
        return;
    if (rr_simulation_get_relations(simulation, potential)->team ==
        rr_simulation_team_id_mobs)
        return;
    if (rr_simulation_get_health(simulation, potential)->health == 0)
        return;
    struct rr_component_physical *t_physical =
        rr_simulation_get_physical(simulation, potential);
    struct rr_vector delta = {captures->x - t_physical->x,
                              captures->y - t_physical->y};
    float dist = rr_vector_get_magnitude(&delta);
    if (dist > captures->closest_dist)
        return;
    captures->done = 1;
}

static int too_close(struct rr_simulation *this, float x, float y, float r)
{
    struct too_close_captures shg_captures = {this, x, y, r, 0};
    struct rr_spatial_hash *shg =
        &rr_simulation_get_arena(this, 1)->spatial_hash;
    rr_spatial_hash_query(shg, x, y, r, r, &shg_captures, too_close_cb);
    return shg_captures.done;
}

static void spawn_mob(struct rr_simulation *this, uint32_t grid_x,
                      uint32_t grid_y)
{
    struct rr_component_arena *arena = rr_simulation_get_arena(this, 1);
    struct rr_maze_grid *grid =
        rr_component_arena_get_grid(arena, grid_x, grid_y);
    uint8_t id;
    if (grid->spawn_function != NULL && rr_frand() < 1)
    {
        id = grid->spawn_function();
        if (id == ALL_MOBS)
            id = get_spawn_id(RR_GLOBAL_BIOME, grid);
        else if (id == DIFFICULT_MOBS)
            for (uint8_t i = 0; i < 10; ++i)
            {
                id = get_spawn_id(RR_GLOBAL_BIOME, grid);
                if (id != rr_mob_id_dakotaraptor &&
                    id != rr_mob_id_ornithomimus &&
                    id != rr_mob_id_triceratops &&
                    id != rr_mob_id_fern &&
                    id != rr_mob_id_meteor)
                    break;
            }
    }
    else
        id = get_spawn_id(RR_GLOBAL_BIOME, grid);
    uint8_t rarity =
        get_spawn_rarity(grid->difficulty + grid->local_difficulty * 0);
    if (!should_spawn_at(id, rarity))
        return;
    for (uint32_t n = 0; n < 10; ++n)
    {
        struct rr_vector pos = {(grid_x + rr_frand()) * arena->maze->grid_size,
                                (grid_y + rr_frand()) * arena->maze->grid_size};
        if (too_close(this, pos.x, pos.y,
                      RR_MOB_DATA[id].radius *
                              RR_MOB_RARITY_SCALING[rarity].radius +
                          500))
            continue;
        EntityIdx mob_id = rr_simulation_alloc_mob(
            this, 1, pos.x, pos.y, id, rarity, rr_simulation_team_id_mobs);
        rr_simulation_get_mob(this, mob_id)->zone = grid;
        grid->grid_points += RR_MOB_DIFFICULTY_COEFFICIENTS[id];
        grid->spawn_timer = 0;
        break;
    }
}

#define PLAYER_COUNT_CAP (12)

static void count_flower_vicinity(EntityIdx entity, void *_simulation)
{
    struct rr_simulation *this = _simulation;
    struct rr_component_arena *arena = rr_simulation_get_arena(this, 1);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(this, entity);
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(this, relations->owner);
    if (is_dead_flower(this, entity))
        return;
    if (physical->bubbling_to_death)
        return;
    if (player_info->client->disconnected)
        return;
    if (dev_cheat_enabled(this, entity, no_grid_influence))
        return;
#define FOV 3072
    uint32_t sx = rr_fclamp((physical->x - FOV) / arena->maze->grid_size, 0,
                            arena->maze->maze_dim - 1);
    uint32_t sy = rr_fclamp((physical->y - FOV) / arena->maze->grid_size, 0,
                            arena->maze->maze_dim - 1);
    uint32_t ex = rr_fclamp((physical->x + FOV) / arena->maze->grid_size, 0,
                            arena->maze->maze_dim - 1);
    uint32_t ey = rr_fclamp((physical->y + FOV) / arena->maze->grid_size, 0,
                            arena->maze->maze_dim - 1);
#undef FOV
    uint32_t level = rr_simulation_get_flower(_simulation, entity)->level;
    for (uint32_t x = sx; x <= ex; ++x)
        for (uint32_t y = sy; y <= ey; ++y)
        {
            struct rr_maze_grid *grid =
                rr_component_arena_get_grid(arena, x, y);
            grid->player_count += grid->player_count < PLAYER_COUNT_CAP;
            grid->local_difficulty +=
                rr_fclamp((level - (grid->difficulty - 1) * 2.1) / 10, -1, 1);
        }
}

static void despawn_mob(EntityIdx entity, void *_simulation)
{
    struct rr_simulation *this = _simulation;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    if (physical->arena != 1)
        return;
    if (rr_simulation_has_arena(this, entity))
        return;
    struct rr_component_arena *arena = rr_simulation_get_arena(this, 1);
    struct rr_component_mob *mob = rr_simulation_get_mob(this, entity);
    struct rr_component_ai *ai = rr_simulation_get_ai(this, entity);
    if (mob->player_spawned)
        return;
    if (rr_component_arena_get_grid(
            arena,
            rr_fclamp(physical->x / arena->maze->grid_size, 0,
                      arena->maze->maze_dim - 1),
            rr_fclamp(physical->y / arena->maze->grid_size, 0,
                      arena->maze->maze_dim - 1))
            ->player_count == 0)
    {
        if (mob->ticks_to_despawn > 30 * 25)
            mob->ticks_to_despawn = 30 * 25;
        if (--mob->ticks_to_despawn == 0)
        {
            mob->no_drop = 1;
            rr_simulation_request_entity_deletion(this, entity);
        }
    }
    else
        mob->ticks_to_despawn = 30 * 25;
}

static float get_max_points(struct rr_simulation *this,
                            struct rr_maze_grid *grid)
{
    float coeff = rr_simulation_get_arena(this, 1)->pvp ? 0.3 : 3;
    return coeff * (0.2 + (grid->player_count) * 1.2) *
           powf(1.1, grid->overload_factor);
}
static int tick_grid(struct rr_simulation *this, struct rr_maze_grid *grid,
                     uint32_t grid_x, uint32_t grid_y)
{
    if (grid->value == 0 || (grid->value & 8))
        return 0;
    grid->local_difficulty =
        rr_fclamp(grid->local_difficulty, -0.5, PLAYER_COUNT_CAP);
    if (grid->local_difficulty > 0)
    {
        grid->overload_factor = rr_fclamp(
            grid->overload_factor + 0.005 * grid->local_difficulty / 25, 0,
            1.5 * grid->local_difficulty);
    }
    else
    {
        grid->overload_factor = rr_fclamp(grid->overload_factor - 0.025 / 25, 0,
                                          grid->overload_factor);
    }
    float player_modifier = 1 + grid->player_count * 4.0 / 3;
    float difficulty_modifier = 150 + 3 * grid->difficulty;
    float overload_modifier =
        powf(1.2, grid->local_difficulty + grid->overload_factor);
    float max_points = get_max_points(this, grid);
    if (grid->grid_points >= max_points)
        return 0;
    float base_modifier = (max_points) / (max_points - grid->grid_points);
    float spawn_at = base_modifier * difficulty_modifier * overload_modifier /
                     (player_modifier);
    if (grid->player_count == 0)
    {
        grid->overload_factor =
            rr_fclamp(grid->overload_factor - 0.025 / 25, 0, 15);
        grid->spawn_timer = rr_frand() * 0.75 * spawn_at;
    }
    else if (grid->spawn_timer >= spawn_at)
    {
        spawn_mob(this, grid_x, grid_y);
        return 1;
    }
    else
        ++grid->spawn_timer;
    return 0;
}

static void tick_maze(struct rr_simulation *this)
{
    struct rr_component_arena *arena = rr_simulation_get_arena(this, 1);
    for (uint32_t i = 0; i < arena->maze->maze_dim * arena->maze->maze_dim; ++i)
        arena->maze->maze[i].local_difficulty =
            arena->maze->maze[i].player_count = 0;

    rr_simulation_for_each_flower(this, this, count_flower_vicinity);
    rr_simulation_for_each_mob(this, this, despawn_mob);
    for (uint32_t grid_x = 0; grid_x < arena->maze->maze_dim; grid_x += 2)
    {
        for (uint32_t grid_y = 0; grid_y < arena->maze->maze_dim; grid_y += 2)
        {
            struct rr_maze_grid *nw =
                rr_component_arena_get_grid(arena, grid_x, grid_y);
            struct rr_maze_grid *ne =
                rr_component_arena_get_grid(arena, grid_x + 1, grid_y);
            struct rr_maze_grid *sw =
                rr_component_arena_get_grid(arena, grid_x, grid_y + 1);
            struct rr_maze_grid *se =
                rr_component_arena_get_grid(arena, grid_x + 1, grid_y + 1);
            float max_overall =
                get_max_points(this, nw) + get_max_points(this, ne) +
                get_max_points(this, sw) + get_max_points(this, se);
            if (nw->grid_points + ne->grid_points + sw->grid_points +
                    se->grid_points >
                max_overall)
                continue;
            if (tick_grid(this, nw, grid_x, grid_y))
                continue;
            if (tick_grid(this, ne, grid_x + 1, grid_y))
                continue;
            if (tick_grid(this, sw, grid_x, grid_y + 1))
                continue;
            if (tick_grid(this, se, grid_x + 1, grid_y + 1))
                continue;
        }
    }
}

#define RR_TIME_BLOCK_(_, CODE)                                                \
    {                                                                          \
        struct timeval start;                                                  \
        struct timeval end;                                                    \
        gettimeofday(&start, NULL);                                            \
        CODE;                                                                  \
        gettimeofday(&end, NULL);                                              \
        uint64_t elapsed_time = (end.tv_sec - start.tv_sec) * 1000000 +        \
                                (end.tv_usec - start.tv_usec);                 \
        if (elapsed_time > 3000)                                               \
        {                                                                      \
            printf(_ " took %lu microseconds with %d entities\n",              \
                   elapsed_time, this->physical_count);                        \
        }                                                                      \
    };

#define RR_TIME_BLOCK(_, CODE)                                                 \
    {                                                                          \
        CODE;                                                                  \
    };

static int64_t last_zone_epoch = -1;

void rr_simulation_tick(struct rr_simulation *this)
{
    rr_simulation_create_component_vectors(this);
    RR_TIME_BLOCK("collision_detection",
                  { rr_system_collision_detection_tick(this); });
    RR_TIME_BLOCK("ai", { rr_system_ai_tick(this); });
    RR_TIME_BLOCK("drops", { rr_system_drops_tick(this); });
    RR_TIME_BLOCK("petal_behavior", { rr_system_petal_behavior_tick(this); });
    RR_TIME_BLOCK("collision_resolution",
                  { rr_system_collision_resolution_tick(this); });
    RR_TIME_BLOCK("web", { rr_system_web_tick(this); });
    RR_TIME_BLOCK("velocity", { rr_system_velocity_tick(this); });
    RR_TIME_BLOCK("centipede", { rr_system_centipede_tick(this); });
    RR_TIME_BLOCK("health", { rr_system_health_tick(this); });
    RR_TIME_BLOCK("camera", { rr_system_camera_tick(this); });
    RR_TIME_BLOCK("checkpoints", { rr_system_checkpoints_tick(this); });
    RR_TIME_BLOCK("spawn_tick", { tick_maze(this); });
    memcpy(this->deleted_last_tick, this->pending_deletions,
           sizeof this->pending_deletions);
    memset(this->pending_deletions, 0, sizeof this->pending_deletions);
    RR_TIME_BLOCK("free_component", {
        rr_bitset_for_each_bit(
            this->deleted_last_tick,
            this->deleted_last_tick + (RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT)),
            this, __rr_simulation_pending_deletion_free_components);
    });
    RR_TIME_BLOCK("unset_entity", {
        rr_bitset_for_each_bit(
            this->deleted_last_tick,
            this->deleted_last_tick + RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT),
            this, __rr_simulation_pending_deletion_unset_entity);
    });
}

int rr_simulation_entity_alive(struct rr_simulation *this, EntityHash hash)
{
    return this->entity_tracker[(EntityIdx)hash] &&
           this->entity_hash_tracker[(EntityIdx)hash] == (hash >> 16) &&
           !rr_bitset_get(this->deleted_last_tick, (EntityIdx)hash);
}

EntityHash rr_simulation_get_entity_hash(struct rr_simulation *this,
                                         EntityIdx id)
{
    return ((uint32_t)(this->entity_hash_tracker[id]) << 16) | id;
}
