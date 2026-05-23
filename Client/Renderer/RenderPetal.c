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

#include <Client/Renderer/ComponentRender.h>

#include <math.h>

#include <Client/Game.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>

#include <Client/Assets/RenderFunctions.h>

void rr_component_petal_render(EntityIdx entity, struct rr_game *game,
                               struct rr_simulation *simulation)
{
    struct rr_renderer *renderer = game->renderer;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, entity);
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, entity);
    rr_renderer_set_global_alpha(renderer, 1 - physical->deletion_animation);
    rr_renderer_scale(renderer, 1 + physical->deletion_animation * 0.5);
    if (petal->id == rr_petal_id_uranium && physical->on_title_screen)
    {
        rr_renderer_set_fill(renderer, 0x2063bf2e);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 0,
                        (25 + 10 * (petal->rarity > 1 ? petal->rarity - 1 : 1) *
                            (1 + sinf(physical->animation_timer * 3))) *
                            physical->radius / 15);
        rr_renderer_fill(renderer);
    }
    if (petal->rarity >= rr_rarity_id_exotic)
    {
        struct rr_particle_manager *particle_manager =
            petal->id != rr_petal_id_meteor
                ? &game->default_particle_manager
                : &game->foreground_particle_manager;
        float exotic_coeff = petal->rarity == rr_rarity_id_exotic ? 0.5 : 1;
        float size_coeff =
            physical->on_title_screen ? physical->radius / 20 : 1;
        float colorful_coeff = petal->id == rr_petal_id_fireball ||
                               petal->id == rr_petal_id_meteor ? 2 : 1;
        float pos_offset = 0;
        if (physical->on_title_screen)
        {
            if (petal->id == rr_petal_id_magnet ||
                petal->id == rr_petal_id_crest ||
                petal->id == rr_petal_id_bubble ||
                petal->id == rr_petal_id_meteor)
                pos_offset = physical->radius * rr_frand();
        }
        struct rr_simulation_animation *particle =
            rr_particle_alloc(particle_manager, rr_animation_type_default);
        particle->x = physical->lerp_x;
        particle->y = physical->lerp_y;
        if (pos_offset > 0)
        {
            struct rr_vector vector;
            rr_vector_from_polar(&vector, pos_offset, 2 * M_PI * rr_frand());
            particle->x += vector.x;
            particle->y += vector.y;
        }
        rr_vector_from_polar(&particle->velocity,
                             (3 + 2 * rr_frand()) * exotic_coeff * size_coeff,
                             2 * M_PI * rr_frand());
        particle->friction = 0.9;
        particle->size = (3 + 2 * rr_frand()) * exotic_coeff * size_coeff;
        particle->opacity = (0.3 + 0.2 * rr_frand()) *
                                exotic_coeff * colorful_coeff;
        particle->disappearance = physical->on_title_screen ? 4 : 6;
        if (petal->rarity == rr_rarity_id_ultimate) {particle->color = 0xffff00ff;}
        if (petal->rarity == rr_rarity_id_quantum) {particle->color = 0xfface3df;}
        if (petal->rarity == rr_rarity_id_aurous) {particle->color = 0xffd1ab38;}
        if (petal->rarity == rr_rarity_id_eternal) {particle->color = 0xff8d9ac9;}
        if (petal->rarity == rr_rarity_id_hyper) {particle->color = 0xff467330;}
        if (petal->rarity == rr_rarity_id_sunshine) {particle->color = 0xffc29c5b;}
        if (petal->rarity == rr_rarity_id_nebula) {particle->color = 0xff4914a6;}
        if (petal->rarity == rr_rarity_id_infinity) {particle->color = 0xff3d3d3d;}
        if (petal->rarity == rr_rarity_id_calamity) {particle->color = 0xff852121;}
        if (petal->rarity == rr_rarity_id_unique) {particle->color = 0xffeeeeee;}
        if (petal->rarity == rr_rarity_id_cosmic) {particle->color = 0xff692c9b;}
        if (petal->rarity == rr_rarity_id_galactic) {particle->color = 0xffc0727d;}
        if (petal->rarity == rr_rarity_id_ethereal) {particle->color = 0xff692c9b;}
        if (petal->rarity == rr_rarity_id_prime) {particle->color = 0xffc0727d;}

        if (petal->id == rr_petal_id_fireball)
        {
            switch (rand() % 3)
            {
            case 0:
                particle->color = 0xffbc0303;
                break;
            case 1:
                particle->color = 0xffce5d0b;
                break;
            case 2:
                particle->color = 0xffd4cc08;
                break;
            }
        }
        else if (petal->id == rr_petal_id_meteor)
            particle->color = 0xffab3423;
    }
    if (game->cache.tint_petals)
        rr_renderer_add_color_filter(renderer, RR_RARITY_COLORS[petal->rarity],
                                     0.4);
    rr_renderer_add_color_filter(renderer, 0xffff0000,
                                 0.5 * health->damage_animation);
    rr_renderer_rotate(renderer, physical->lerp_angle);

    rr_renderer_scale(renderer, physical->radius / 10);
    uint8_t use_cache =
        (((health->damage_animation < 0.1) | game->cache.low_performance_mode) &
         1) &
        (1 - game->cache.tint_petals);
    if (petal->id != rr_petal_id_peas || petal->detached == 1)
        rr_renderer_draw_petal(renderer, petal->id, use_cache);
    else
        rr_renderer_draw_static_petal(renderer, petal->id, petal->rarity,
                                      use_cache);
}