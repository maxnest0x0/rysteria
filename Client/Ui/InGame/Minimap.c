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

#include <Client/Assets/RenderFunctions.h>
#include <Client/Game.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Shared/StaticData.h>

struct rr_renderer minimap;

static uint8_t previous_biome = 255;

#define DRAW_MINIMAP(renderer, grid)                                           \
    if (arena->biome != previous_biome)                                        \
    {                                                                          \
        previous_biome = arena->biome;                                         \
        float s = floorf(this->abs_width / maze_dim);                          \
        rr_renderer_set_dimensions(renderer, s *maze_dim, s *maze_dim);        \
        (renderer)->state.filter.color = 0xffffffff;                           \
        for (int32_t x = 0; x < maze_dim; ++x)                                 \
            for (int32_t y = 0; y < maze_dim; ++y)                             \
            {                                                                  \
                uint8_t at = grid[y * maze_dim + x].value;                     \
                uint8_t difficulty = grid[y * maze_dim + x].difficulty / 4;    \
                if (at == 1)                                                   \
                {                                                              \
                    if (difficulty % 2 == 0)                                   \
                        (renderer)->state.filter.amount =                      \
                            (difficulty >= 4 && difficulty <= 8) ||            \
                            difficulty == 12 ? 0.3 : 0.5;                      \
                    rr_renderer_set_fill(                                      \
                        renderer, RR_RARITY_COLORS[difficulty / 2]);           \
                    rr_renderer_begin_path(renderer);                          \
                    rr_renderer_fill_rect(renderer, x *s, y *s, s, s);         \
                }                                                              \
                else if (at != 0)                                              \
                {                                                              \
                    for (int8_t i = -1; i <= 1; ++i)                           \
                        for (int8_t j = -1; j <= 1; ++j)                       \
                    {                                                          \
                        if (x + i < 0 || x + i >= maze_dim ||                  \
                            y + j < 0 || y + j >= maze_dim)                    \
                            continue;                                          \
                        uint8_t potential =                                    \
                            grid[(y + j) * maze_dim + (x + i)].difficulty / 4; \
                        if (potential > difficulty)                            \
                            difficulty = potential;                            \
                    }                                                          \
                    uint8_t left = (at >> 1) & 1;                              \
                    uint8_t top = at & 1;                                      \
                    uint8_t inverse = (at >> 3) & 1;                           \
                    if (difficulty % 2 == 0)                                   \
                        (renderer)->state.filter.amount =                      \
                            (difficulty >= 4 && difficulty <= 8) ||            \
                            difficulty == 12 ? 0.3 : 0.5;                      \
                    rr_renderer_set_fill(                                      \
                        renderer, RR_RARITY_COLORS[difficulty / 2]);           \
                    rr_renderer_begin_path(renderer);                          \
                    rr_renderer_move_to(renderer, (x + inverse ^ left) * s,    \
                                        (y + inverse ^ top) * s);              \
                    float start_angle = 0;                                     \
                    if (top == 0 && left == 1)                                 \
                        start_angle = M_PI / 2;                                \
                    else if (top == 1 && left == 1)                            \
                        start_angle = M_PI;                                    \
                    else if (top == 1 && left == 0)                            \
                        start_angle = M_PI * 3 / 2;                            \
                    rr_renderer_partial_arc(renderer, (x + left) * s,          \
                                            (y + top) * s, s, start_angle,     \
                                            start_angle + M_PI / 2, 0);        \
                    rr_renderer_fill(renderer);                                \
                }                                                              \
                (renderer)->state.filter.amount = 0;                           \
            }                                                                  \
    }

static void minimap_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    if (!game->simulation_ready)
        return;
    struct rr_renderer *renderer = game->renderer;
    struct rr_component_arena *arena =
        rr_simulation_get_arena(game->simulation, game->player_info->arena);
    float grid_size = RR_MAZES[arena->biome].grid_size;
    uint32_t maze_dim = RR_MAZES[arena->biome].maze_dim;
    struct rr_maze_grid *grid = RR_MAZES[arena->biome].maze;
    DRAW_MINIMAP(&minimap, grid);
    rr_renderer_scale(renderer, renderer->scale);
    rr_renderer_scale(renderer, this->abs_width / minimap.width);
    rr_renderer_draw_image(renderer, &minimap);
    rr_renderer_scale(renderer, minimap.width / this->abs_width);
    rr_renderer_set_fill(renderer, 0xffff00ff);
    renderer->state.filter.amount = 0.2;
    rr_renderer_set_stroke(renderer, 0xffff00ff);
    rr_renderer_set_line_width(renderer, 2);
    for (uint32_t i = RR_SQUAD_MEMBER_COUNT; i > 0; --i)
    {
        if (game->player_infos[i - 1] == RR_NULL_ENTITY)
            continue;
        struct rr_component_player_info *player_info =
            rr_simulation_get_player_info(game->simulation,
                                          game->player_infos[i - 1]);
        if (player_info->arena != game->player_info->arena)
            continue;
        if (player_info->flower_id == RR_NULL_ENTITY)
            continue;
        struct rr_component_physical *physical =
            rr_simulation_get_physical(game->simulation,
                                       player_info->flower_id);
        if (i == 1)
        {
            rr_renderer_set_stroke(renderer, 0xff0080ff);
            renderer->state.filter.amount = 0;
            rr_renderer_set_fill(renderer, 0xff0080ff);
        }
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(
            renderer,
            this->abs_width *
                (physical->lerp_x / (grid_size * maze_dim) - 0.5),
            this->abs_height *
                (physical->lerp_y / (grid_size * maze_dim) - 0.5),
            4);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
    }
}

static void minimap_redraw(void *captures)
{
    previous_biome = 255;
}

struct rr_ui_element *rr_ui_minimap_init(struct rr_game *game)
{
    struct rr_ui_element *this = rr_ui_element_init();

    this->abs_width = this->width = this->abs_height = this->height = 250;
    this->on_render = minimap_on_render;
    rr_renderer_init(&minimap);
    minimap.on_context_restore = minimap_redraw;
    return this;
}