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

#include <stdlib.h>

#include <Server/Client.h>
#include <Server/Server.h>
#include <Server/Simulation.h>

struct drop_pick_up_captures
{
    struct rr_simulation *simulation;
    struct rr_component_player_info *player_info;
    struct rr_component_physical *flower_physical;
    float closest_dist;
    EntityIdx closest_drop;
};

static void drop_cb(EntityIdx entity, void *_captures)
{
    struct drop_pick_up_captures *captures = _captures;
    struct rr_simulation *this = captures->simulation;

    struct rr_component_drop *drop = rr_simulation_get_drop(this, entity);
    if (drop->ticks_until_despawn > 25 * 10 * (drop->rarity + 1) - 10)
        return;
    if (drop->ticks_until_despawn == 0)
        return;

    uint8_t i = captures->player_info->client - this->server->clients;
    if (rr_bitset_get_bit(drop->can_be_picked_up_by, i) == 0)
        return;
    if (rr_bitset_get_bit(drop->picked_up_by, i))
        return;

    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    struct rr_vector delta = {physical->x - captures->flower_physical->x,
                              physical->y - captures->flower_physical->y};
    if (rr_vector_magnitude_cmp(
        &delta, captures->closest_dist + physical->radius) == 1)
        return;

    captures->closest_dist = rr_vector_get_magnitude(&delta) - physical->radius;
    captures->closest_drop = entity;
}

static void drop_pick_up(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    if (is_dead_flower(this, entity))
        return;

    struct rr_component_relations *flower_relations =
        rr_simulation_get_relations(this, entity);
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(this, flower_relations->owner);
    if (rr_simulation_entity_alive(this, flower_relations->owner) == 0)
        return;
    if (player_info->client->disconnected)
        return;
    if (player_info->drops_this_tick_size >= 1)
        return;

    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(this, entity);
    if (flower_physical->bubbling_to_death)
        return;

    struct rr_component_arena *arena =
        rr_simulation_get_arena(this, flower_physical->arena);
    struct drop_pick_up_captures captures;
    captures.simulation = this;
    captures.player_info = player_info;
    captures.flower_physical = flower_physical;
    captures.closest_dist =
        flower_physical->radius + player_info->modifiers.drop_pickup_radius;
    captures.closest_drop = RR_NULL_ENTITY;
    rr_spatial_hash_query(&arena->spatial_hash, flower_physical->x,
                          flower_physical->y, captures.closest_dist,
                          captures.closest_dist, &captures, drop_cb);
    if (captures.closest_drop == RR_NULL_ENTITY)
        return;

    struct rr_component_drop *drop =
        rr_simulation_get_drop(this, captures.closest_drop);
    uint8_t i = player_info->client - this->server->clients;
    rr_bitset_set(drop->picked_up_by, i);
    ++player_info
          ->collected_this_run[drop->id * rr_rarity_id_max + drop->rarity];
    rr_component_player_info_set_update_loot(player_info);
    player_info->drops_this_tick[player_info->drops_this_tick_size].id =
        drop->id;
    player_info->drops_this_tick[player_info->drops_this_tick_size].rarity =
        drop->rarity;
    ++player_info->drops_this_tick_size;
}

static void drop_despawn_tick(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_drop *drop = rr_simulation_get_drop(this, entity);
    if (drop->ticks_until_despawn == 0)
    {
        rr_simulation_request_entity_deletion(this, entity);
        return;
    }
    --drop->ticks_until_despawn;
}

void rr_system_drops_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_drop(this, this, drop_despawn_tick);
    rr_simulation_for_each_flower(this, this, drop_pick_up);
}
