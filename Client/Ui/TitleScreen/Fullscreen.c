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

#include <Client/DOM.h>
#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Ui/Engine.h>

static uint8_t fullscreen_toggle_button_should_show(struct rr_ui_element *this,
                                                    struct rr_game *game)
{
    return game->is_mobile;
}

static void fullscreen_toggle_button_on_render(struct rr_ui_element *this,
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
    rr_renderer_set_line_width(renderer, this->abs_width / 12);
    rr_renderer_set_line_cap(renderer, 1);
    rr_renderer_set_stroke(renderer, 0xffffffff);
    uint8_t enabled = rr_dom_fullscreen_enabled();
    for (uint8_t i = 0; i < 4; ++i)
    {
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, -this->abs_width / 12,
                            -this->abs_height * 0.25);
        rr_renderer_line_to(
            renderer,
            enabled ? -this->abs_width / 12 : -this->abs_width * 0.25,
            enabled ? -this->abs_height / 12 : -this->abs_height * 0.25);
        rr_renderer_line_to(renderer, -this->abs_width * 0.25,
                            -this->abs_height / 12);
        rr_renderer_stroke(renderer);
        rr_renderer_rotate(renderer, M_PI * 0.5);
    }
}

static void fullscreen_toggle_button_on_event(struct rr_ui_element *this,
                                              struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (game->pressed != this)
            return;
        if (rr_dom_fullscreen_enabled())
            rr_dom_exit_fullscreen();
        else
            rr_dom_request_fullscreen();
    }
    rr_ui_render_tooltip_below(this, game->fullscreen_tooltip, game);
    game->cursor = rr_game_cursor_pointer;
}

struct rr_ui_element *rr_ui_fullscreen_toggle_button_init()
{
    struct rr_ui_element *this = rr_ui_element_init();
    rr_ui_set_background(this, 0x80888888);
    this->abs_width = this->abs_height = this->width = this->height = 40;
    this->on_event = fullscreen_toggle_button_on_event;
    this->on_render = fullscreen_toggle_button_on_render;
    this->should_show = fullscreen_toggle_button_should_show;
    return this;
}
