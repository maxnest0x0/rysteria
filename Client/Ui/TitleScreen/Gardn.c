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
#include <string.h>

#include <Client/DOM.h>
#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Ui/Engine.h>

#include <Shared/Utilities.h>

struct rr_renderer flower_icon;

static void rr_renderer_flower_icon_draw(struct rr_renderer_filter filter)
{
    rr_renderer_set_dimensions(&flower_icon, 0, 0);
    rr_renderer_set_dimensions(&flower_icon, 54, 54);
    rr_renderer_set_transform(&flower_icon, 1, 0, 0, 0, 1, 0);
    rr_renderer_translate(&flower_icon, 27, 27);
    flower_icon.state.filter = filter;
    struct rr_renderer *renderer = &flower_icon;
    rr_renderer_set_stroke(renderer, 0xffffffff);
    rr_renderer_set_fill(renderer, 0xffffffff);
    rr_renderer_set_line_width(renderer, 3);
    rr_renderer_begin_path(renderer);
    rr_renderer_arc(renderer, 0, 0, 25);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_set_global_composite_operation(renderer, 1);
    rr_renderer_set_line_width(renderer, 1);
    rr_renderer_begin_path(renderer);
    rr_renderer_ellipse(renderer, -7, -5, 3, 6);
    rr_renderer_move_to(renderer, 7, -5);
    rr_renderer_ellipse(renderer, 7, -5, 3, 6);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
    rr_renderer_context_state_free(renderer, &state);
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_begin_path(renderer);
    rr_renderer_ellipse(renderer, -7, -5, 3, 6);
    rr_renderer_ellipse(renderer, 7, -5, 3, 6);
    rr_renderer_clip(renderer);
    rr_renderer_begin_path(renderer);
    rr_renderer_arc(renderer, -7 + cosf(-1) * 2, -5 + sinf(-1) * 4, 3);
    rr_renderer_arc(renderer, 7 + cosf(-1) * 2, -5 + sinf(-1) * 4, 3);
    rr_renderer_fill(renderer);
    rr_renderer_context_state_free(renderer, &state);
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_set_global_composite_operation(renderer, 1);
    rr_renderer_set_line_width(renderer, 1.5);
    rr_renderer_set_line_cap(renderer, 1);
    rr_renderer_begin_path(renderer);
    rr_renderer_move_to(renderer, -6, 10);
    rr_renderer_quadratic_curve_to(renderer, 0, 15, 6, 10);
    rr_renderer_stroke(renderer);
    rr_renderer_context_state_free(renderer, &state);
}

static uint8_t gardn_button_should_show(struct rr_ui_element *this,
                                         struct rr_game *game)
{
    return !game->simulation_ready;
}

static void gardn_toggle_button_on_render(struct rr_ui_element *this,
                                           struct rr_game *game)
{
    struct rr_renderer *renderer = game->renderer;
    if (game->focused == this)
        renderer->state.filter.amount = 0.2;
    rr_renderer_scale(renderer, renderer->scale);
    rr_renderer_set_fill(renderer, this->fill);
    renderer->state.filter.amount += 0.2;
    rr_renderer_begin_path(renderer);
    rr_renderer_round_rect(renderer, -this->abs_width / 2,
                           -this->abs_height / 2, this->abs_width,
                           this->abs_height, 6);
    rr_renderer_fill(renderer);
    rr_renderer_scale(renderer, 0.6);
    rr_renderer_flower_icon_draw(renderer->state.filter);
    rr_renderer_draw_image(renderer, &flower_icon);
}

static void gardn_toggle_button_on_event(struct rr_ui_element *this,
                                          struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (game->pressed != this)
            return;
        rr_page_open("https://rysteria.pro/gardn/");
    }
    rr_ui_render_tooltip_below(this, game->gardn_tooltip, game);
    game->cursor = rr_game_cursor_pointer;
}

struct rr_ui_element *rr_ui_gardn_toggle_button_init()
{
    rr_renderer_init(&flower_icon);
    struct rr_ui_element *this = rr_ui_element_init();
    rr_ui_set_background(this, 0x80888888);
    this->abs_width = this->abs_height = this->width = this->height = 40;
    this->on_event = gardn_toggle_button_on_event;
    this->on_render = gardn_toggle_button_on_render;
    this->should_show = gardn_button_should_show;
    return this;
}
