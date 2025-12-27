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

#include <math.h>

#include <Client/Game.h>
#include <Client/Ui/Engine.h>
#include <Client/Ui/Ui.h>

static uint8_t anti_afk_container_should_show(struct rr_ui_element *this,
                                              struct rr_game *game)
{
    return game->simulation_ready && game->afk;
}

static void anti_afk_container_animate(struct rr_ui_element *this,
                                       struct rr_game *game)
{
    struct rr_ui_container_metadata *data = this->data;
    struct rr_vector *vector = data->data;
    this->x += vector->x * game->lerp_delta;
    this->y += vector->y * game->lerp_delta;
    if (this->x * 2 < -game->window->abs_width + this->abs_width ||
        this->x * 2 > game->window->abs_width - this->abs_width)
    {
        vector->x = -vector->x;
        this->x = rr_fclamp(this->x * 2,
                            -game->window->abs_width + this->abs_width,
                            game->window->abs_width - this->abs_width) / 2;
    }
    if (this->y * 2 < -game->window->abs_height + this->abs_height ||
        this->y * 2 > game->window->abs_height - this->abs_height)
    {
        vector->y = -vector->y;
        this->y = rr_fclamp(this->y * 2,
                            -game->window->abs_height + this->abs_height,
                            game->window->abs_height - this->abs_height) / 2;
    }
}

struct rr_ui_element *rr_ui_anti_afk_container_init(struct rr_game *game)
{
    struct rr_ui_element *this =
        rr_ui_set_background(rr_ui_v_container_init(
            rr_ui_container_init(), 10, 10,
            rr_ui_set_justify(rr_ui_text_init("AFK Check", 24, 0xffff4444), -1, -1),
            rr_ui_set_justify(
                rr_ui_h_container_init(rr_ui_container_init(), 0, 0,
                    rr_ui_text_init(
                        "If you are here, send this in chat: ", 16, 0xffffffff),
                    rr_ui_text_init(game->afk_challenge, 16, 0xffffffff),
                    NULL),
            -1, -1),
            rr_ui_set_justify(rr_ui_text_init(
                "Otherwise, you will be kicked soon", 16, 0xffffffff),
            -1, -1),
            NULL), 0x40ffffff);
    struct rr_vector *vector = malloc(sizeof *vector);
    rr_vector_from_polar(vector, 100, rr_frand() * 2 * M_PI);
    struct rr_ui_container_metadata *data = this->data;
    data->data = vector;
    this->prevent_on_event = 1;
    this->should_show = anti_afk_container_should_show;
    this->animate = anti_afk_container_animate;
    return this;
}
