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

#include <Server/Client.h>
#include <Server/Simulation.h>

static void system_for_each_function(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    if (physical->bubbling_to_death)
        return;
    struct rr_component_arena *arena =
        rr_simulation_get_arena(this, physical->arena);
    struct rr_component_relations *relations =
        rr_simulation_get_relations(this, entity);
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(this, relations->owner);
    if (player_info->client->disconnected)
        return;
    if (player_info->client->verified == 0)
        return;
    uint32_t grid_x = rr_fclamp(physical->x / arena->maze->grid_size,
                                0, arena->maze->maze_dim - 1);
    uint32_t grid_y = rr_fclamp(physical->y / arena->maze->grid_size,
                                0, arena->maze->maze_dim - 1);
    for (uint8_t i = 0; i < arena->maze->checkpoint_count; ++i)
    {
        struct rr_checkpoint checkpoint = arena->maze->checkpoints[i];
        if (player_info->level >= checkpoint.min_level &&
            grid_x >= checkpoint.x * 2 &&
            grid_x < (checkpoint.x + checkpoint.w) * 2 &&
            grid_y >= checkpoint.y * 2 &&
            grid_y < (checkpoint.y + checkpoint.h) * 2)
        {
            if (player_info->client->checkpoint != i)
            {
                player_info->client->checkpoint = i;
                rr_server_client_write_to_api(player_info->client);
            }
            break;
        }
    }
}

void rr_system_checkpoints_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_flower(this, this, system_for_each_function);
}
