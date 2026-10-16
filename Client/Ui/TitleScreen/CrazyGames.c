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

#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Platform.h>
#include <Client/Ui/Engine.h>

// The top right of the title screen on CrazyGames: a login button for guests,
// the player's CrazyGames name once they are logged in. Neither exists
// anywhere else.

static uint8_t login_should_show(struct rr_ui_element *this,
                                 struct rr_game *game)
{
    return rr_platform_on_crazygames() && !game->cg_logged_in &&
           !game->simulation_ready;
}

static uint8_t user_should_show(struct rr_ui_element *this,
                                struct rr_game *game)
{
    return rr_platform_on_crazygames() && game->cg_logged_in &&
           !game->simulation_ready;
}

static void login_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_ui_labeled_button_metadata *data = this->data;
    data->clickable = 1;
    if (game->input_data->mouse_buttons_up_this_tick & 1)
        rr_crazygames_login();
    game->cursor = rr_game_cursor_pointer;
}

struct rr_ui_element *rr_ui_crazygames_login_button_init()
{
    struct rr_ui_element *this =
        rr_ui_labeled_button_init("Login with CrazyGames", 28, NULL);
    struct rr_ui_labeled_button_metadata *data = this->data;
    data->clickable = 1;
    this->on_event = login_on_event;
    this->should_show = login_should_show;
    return this;
}

struct rr_ui_element *rr_ui_crazygames_user_init(struct rr_game *game)
{
    struct rr_ui_element *this = rr_ui_text_init(game->cg_label, 16, 0xffffffff);
    this->should_show = user_should_show;
    return this;
}
