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
#include <Client/Renderer/Renderer.h>
#include <Client/Ui/Engine.h>

// The terms and privacy panel, opened from the settings. The same text is in
// crazygames-submission/text/04-privacy-and-moderation.md, keep them in step.

static uint8_t legal_should_show(struct rr_ui_element *this,
                                 struct rr_game *game)
{
    return game->menu_open == rr_game_menu_legal;
}

static void legal_animate(struct rr_ui_element *this, struct rr_game *game)
{
    this->width = this->abs_width;
    this->height = this->abs_height;
    rr_renderer_translate(
        game->renderer, -(this->x + this->abs_width / 2) * 2 * this->animation,
        0);
}

static void open_legal_on_event(struct rr_ui_element *this,
                                struct rr_game *game)
{
    struct rr_ui_labeled_button_metadata *data = this->data;
    data->clickable = 1;
    if (game->input_data->mouse_buttons_up_this_tick & 1)
        game->menu_open = rr_game_menu_legal;
    game->cursor = rr_game_cursor_pointer;
}

struct rr_ui_element *rr_ui_legal_button_init()
{
    struct rr_ui_element *this =
        rr_ui_labeled_button_init("Terms & Privacy", 24, NULL);
    struct rr_ui_labeled_button_metadata *data = this->data;
    data->clickable = 1;
    this->on_event = open_legal_on_event;
    return this;
}

#define LINE(text)                                                             \
    rr_ui_set_justify(rr_ui_text_init(text, 14, 0xffffffff), -1, -1)
#define HEADING(text)                                                          \
    rr_ui_set_justify(rr_ui_text_init(text, 18, 0xffffffff), -1, -1)

// clang-format off
struct rr_ui_element *rr_ui_legal_container_init(struct rr_game *game)
{
    struct rr_ui_element *this = rr_ui_pad(
        rr_ui_set_background(
            rr_ui_v_pad(
                rr_ui_set_justify(
                    rr_ui_v_container_init(rr_ui_container_init(), 10, 5,
                        rr_ui_text_init("Terms & Privacy", 24, 0xffffffff),
                        rr_ui_static_space_init(5),
                        HEADING("Terms"),
                        LINE("Play fair: no cheating, hacking or abusing bugs."),
                        LINE("Be kind in chat and in nicknames. Bad words are hidden."),
                        LINE("The game is free and provided as it is. Progress can be"),
                        LINE("reset or lost, and cheating accounts can be removed."),
                        LINE("Made for players aged 13 and up."),
                        rr_ui_static_space_init(5),
                        HEADING("Privacy"),
                        LINE("Your progress is saved on our server under a random id"),
                        LINE("that is also kept in your browser."),
                        LINE("If you link a CrazyGames or Discord account we also keep"),
                        LINE("that account's id, so your progress follows you."),
                        LINE("Your nickname and chat messages are shown to other"),
                        LINE("players, and chat messages are logged."),
                        LINE("Your IP address is used briefly to stop abuse and appears"),
                        LINE("in our server logs."),
                        LINE("We do not sell or share your data and use no ads or"),
                        LINE("tracking of our own."),
                        LINE("To ask a question or have your data deleted, join our"),
                        LINE("Discord (the button on the title screen)."),
                        rr_ui_static_space_init(5),
                        HEADING("Source code"),
                        LINE("Rysteria is open source under the AGPL-3.0 licence:"),
                        LINE("github.com/parshwa282011/rrolf"),
                        NULL
                    ), -1, -1
                ), 50
            ), 0x40ffffff
        ), 10
    );
    this->animate = legal_animate;
    this->should_show = legal_should_show;
    return this;
}
// clang-format on
