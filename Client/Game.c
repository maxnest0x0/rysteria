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

#include <Client/Game.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifndef EMSCRIPTEN
#include <libwebsockets.h>
#endif

#include <Client/Assets/Init.h>
#include <Client/Assets/RenderFunctions.h>
#include <Client/DOM.h>
#include <Client/InputData.h>
#include <Client/Mobile.h>
#include <Client/Renderer/ComponentRender.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Socket.h>
#include <Client/Storage.h>
#include <Client/System/ParticleRender.h>
#include <Client/Ui/Engine.h>
#include <Shared/Api.h>
#include <Shared/Binary.h>
#include <Shared/Bitset.h>
#include <Shared/Component/Arena.h>
#include <Shared/Component/Flower.h>
#include <Shared/Component/Petal.h>
#include <Shared/Component/Physical.h>
#include <Shared/Component/PlayerInfo.h>
#include <Shared/Crypto.h>
#include <Shared/Rivet.h>
#include <Shared/Utilities.h>
#include <Shared/cJSON.h>
#include <Shared/pb.h>
#include <Shared/MagicNumber.h>

static void rr_game_validate_loadout(struct rr_game *this)
{
    memset(this->loadout_counts, 0, sizeof this->loadout_counts);
    for (uint8_t i = 0; i < RR_MAX_SLOT_COUNT * 2; ++i)
    {
        uint8_t id = this->cache.loadout[i].id;
        if (id == 0)
            continue;
        uint8_t rarity = this->cache.loadout[i].rarity;
        if (this->loadout_counts[id][rarity] >= this->inventory[id][rarity] ||
            (i % RR_MAX_SLOT_COUNT) >= this->slots_unlocked)
            this->cache.loadout[i].id = this->cache.loadout[i].rarity = 0;
        else
            ++this->loadout_counts[id][rarity];
    }
}

static void rr_game_read_account(struct rr_game *this, struct proto_bug *decoder)
{
    memset(this->inventory, 0, sizeof this->inventory);
    memset(this->failed_crafts, 0, sizeof this->failed_crafts);
    memset(this->cache.mob_kills, 0, sizeof this->cache.mob_kills);
    this->cache.experience = proto_bug_read_float64(decoder, "xp");
    uint8_t id;
    while ((id = proto_bug_read_uint8(decoder, "id")))
    {
        uint8_t rarity = proto_bug_read_uint8(decoder, "rarity");
        uint32_t count = proto_bug_read_varuint(decoder, "count");
        this->inventory[id][rarity] = count;
    }
    while ((id = proto_bug_read_uint8(decoder, "id")))
    {
        uint8_t rarity = proto_bug_read_uint8(decoder, "rarity");
        uint32_t count = proto_bug_read_varuint(decoder, "count");
        this->failed_crafts[id][rarity] = count;
    }
    while ((id = proto_bug_read_uint8(decoder, "id")))
    {
        uint8_t rarity = proto_bug_read_uint8(decoder, "rarity");
        uint32_t count = proto_bug_read_varuint(decoder, "count");
        this->cache.mob_kills[id - 1][rarity] = count;
    }
}

uint32_t rr_game_get_adjusted_inventory_count(struct rr_game *this, uint8_t id,
                                              uint8_t rarity)
{
    uint32_t cnt =
        this->inventory[id][rarity] - this->loadout_counts[id][rarity];
    if (id == this->crafting_data.crafting_id)
    {
        if (rarity == this->crafting_data.crafting_rarity)
            cnt -= this->crafting_data.count;
        else if (rarity == this->crafting_data.crafting_rarity + 1)
            cnt -= this->crafting_data.success_count;
    }
    return cnt;
}

static void rr_game_update_significant_rarity(struct rr_game *this)
{
    uint32_t count = 0;
    for (uint8_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
        for (uint8_t id = 1; id < rr_petal_id_max; ++id)
        {
            count += this->inventory[id][rarity];
            if (count >= this->slots_unlocked * 2)
            {
                this->significant_rarity = rarity;
                count = 0;
                break;
            }
        }
}

static void rr_game_crafting_tick(struct rr_game *this, float delta)
{
    if (this->crafting_data.animation > 0)
    {
        this->crafting_data.animation -= delta;
        if (this->crafting_data.animation < 0)
            this->crafting_data.animation = 0;
        if (this->crafting_data.animation == 0 && this->crafting_data.temp_fails)
        {
            uint8_t id = this->crafting_data.crafting_id;
            uint8_t rarity = this->crafting_data.crafting_rarity;
            uint8_t s_rarity = rarity + 1;
            this->cache.experience += this->crafting_data.temp_xp;
            this->failed_crafts[id][rarity] = this->crafting_data.temp_attempts;
            this->inventory[id][rarity] -= this->crafting_data.temp_fails;
            this->crafting_data.count -= this->crafting_data.temp_fails;
            this->inventory[id][s_rarity] += this->crafting_data.temp_successes;
            this->crafting_data.success_count =
                this->crafting_data.temp_successes;
            if (this->crafting_data.temp_successes)
            {
                rr_particle_manager_clear(&this->crafting_particle_manager);
                for (uint8_t i = 0; i < s_rarity * s_rarity; ++i)
                {
                    struct rr_simulation_animation *particle =
                        rr_particle_alloc(&this->crafting_particle_manager,
                                          rr_animation_type_default);
                    particle->x = 60 * (rr_frand() - 0.5);
                    particle->y = 60 * (rr_frand() - 0.5);
                    rr_vector_from_polar(&particle->velocity,
                                         (2 + 8 * rr_frand()) * s_rarity,
                                         -M_PI / 2 + rr_frand() - 0.5);
                    rr_vector_set(&particle->acceleration, 0, 1);
                    particle->friction = 0.9;
                    particle->size = (1 + rr_frand()) * sqrtf(s_rarity);
                    particle->opacity = 1;
                    particle->disappearance = 4;
                    particle->color = RR_RARITY_COLORS[s_rarity];
                }
            }
        }
    }
}

static void rr_game_autocraft_tick(struct rr_game *this, float delta)
{
    if (this->crafting_data.animation == 0)
    {
        this->crafting_data.autocraft_animation -= delta;
        if (this->crafting_data.autocraft_animation < 0 ||
            !this->crafting_data.autocraft)
            this->crafting_data.autocraft_animation = 0;
    }
    if (!this->socket_ready || this->menu_open != rr_game_menu_crafting)
        this->crafting_data.autocraft = 0;
    if (!this->crafting_data.autocraft || this->crafting_data.animation > 0 ||
        this->crafting_data.autocraft_animation > 0)
        return;
    for (uint8_t id = 1; id <= rr_petal_id_meteor; ++id)
    {
        uint32_t sum = 0;
        for (uint8_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
            sum += this->inventory[id][rarity];
        for (uint8_t rarity = 0; rarity < rr_rarity_id_max - 1; ++rarity)
        {
            if (sum < this->slots_unlocked + 4)
                break;
            uint32_t count =
                this->inventory[id][rarity] - this->loadout_counts[id][rarity];
            if (count < 5)
            {
                sum -= this->inventory[id][rarity];
                continue;
            }
            this->crafting_data.crafting_id = id;
            this->crafting_data.crafting_rarity = rarity;
            this->crafting_data.count = sum - this->slots_unlocked + 1;
            if (count < this->crafting_data.count)
                this->crafting_data.count = count;
            this->crafting_data.success_count = 0;
            this->crafting_data.animation = 10;
            this->crafting_data.autocraft_animation = 1;
            this->crafting_data.temp_fails = 0;
            struct proto_bug encoder;
            proto_bug_init(&encoder, RR_OUTGOING_PACKET);
            proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
            proto_bug_write_uint8(&encoder, rr_serverbound_petals_craft,
                                  "header");
            proto_bug_write_uint8(&encoder, this->crafting_data.crafting_id,
                                  "craft id");
            proto_bug_write_uint8(&encoder, this->crafting_data.crafting_rarity,
                                  "craft rarity");
            proto_bug_write_varuint(&encoder, this->crafting_data.count,
                                    "craft count");
            rr_websocket_send(&this->socket, encoder.current - encoder.start);
            return;
        }
    }
    this->crafting_data.autocraft = 0;
    this->crafting_data.count = this->crafting_data.success_count = 0;
    this->crafting_data.crafting_id = this->crafting_data.crafting_rarity = 0;
}

/*
void rr_api_on_get_password(char *s, void *captures)
{
    struct rr_game *this = captures;
    strcpy(this->rivet_account.api_password, s);
    this->logged_in = 1;
    rr_game_connect_socket(this);
}

void rr_rivet_on_log_in(char *token, char *avatar_url, char *name,
                        char *account_number, char *uuid, uint8_t linked,
                        void *captures)
{
    struct rr_game *this = captures;
    strcpy(this->rivet_account.token, token);
    strcpy(this->rivet_account.name, name);
    strcpy(this->rivet_account.avatar_url, avatar_url);
    strcpy(this->rivet_account.account_number, account_number);
    strcpy(this->rivet_account.uuid, uuid);
    this->account_linked = linked;

    // rr_api_get_password(this->rivet_account.token, this);
    rr_api_on_get_password("5d68a8ec6cbf3997a641803260390362d59681bc7524ef3a3fd67afddaba0ba96d1196d30834aa25aa1440cadffb4c87af6495e613c535b793cc1c71aa8c4d04", this);
}
*/

static struct rr_ui_element *make_label_tooltip(char const *text, float size)
{
    return rr_ui_set_background(
        rr_ui_v_container_init(rr_ui_tooltip_container_init(), 10, 10,
                               rr_ui_text_init(text, size, 0xffffffff), NULL),
        0x80000000);
}

static uint8_t simulation_not_ready(struct rr_ui_element *this,
                                    struct rr_game *game)
{
    return 1 - game->simulation_ready;
}

static uint8_t simulation_ready(struct rr_ui_element *this,
                                struct rr_game *game)
{
    return game->simulation_ready;
}

static uint8_t ui_not_hidden(struct rr_ui_element *this,
                             struct rr_game *game)
{
    return !game->cache.hide_ui || !game->simulation_ready;
}

static uint8_t ui_not_hidden_and_simulation_ready(struct rr_ui_element *this,
                                                  struct rr_game *game)
{
    return !game->cache.hide_ui && game->simulation_ready;
}

static uint8_t ui_not_hidden_and_player_dead(struct rr_ui_element *this,
                                             struct rr_game *game)
{
    return (!game->cache.hide_ui && game->flower_dead) ||
            !game->simulation_ready;
}

static uint8_t socket_ready(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->socket_error)
        return 1 + game->socket_error;
    return game->socket_ready;
}

static uint8_t socket_pending_or_ready(struct rr_ui_element *this,
                                       struct rr_game *game)
{
    return game->joined_squad || game->socket_error;
}

static uint8_t player_alive(struct rr_ui_element *this, struct rr_game *game)
{
    return game->simulation_ready && !game->flower_dead;
}

static void window_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        game->menu_open = 0;
        game->chat.chat_active = 0;
    }
}

static void close_menu_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
        game->menu_open = 0;
    game->cursor = rr_game_cursor_pointer;
}

static struct rr_ui_element *close_menu_button_init(float w)
{
    struct rr_ui_element *this = rr_ui_close_button_init(w, close_menu_on_event);
    this->no_reposition = 1;
    rr_ui_pad(rr_ui_set_justify(this, 1, -1), 5);
    return this;
}

static void abandon_game_on_event(struct rr_ui_element *this,
                                  struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        struct proto_bug encoder;
        proto_bug_init(&encoder, RR_OUTGOING_PACKET);
        proto_bug_write_uint8(&encoder, game->socket.quick_verification, "qv");
        proto_bug_write_uint8(&encoder, rr_serverbound_squad_ready, "header");
        rr_websocket_send(&game->socket, encoder.current - encoder.start);
    }
    rr_ui_render_tooltip_below(this, game->abandon_game_tooltip, game);
    game->cursor = rr_game_cursor_pointer;
}

static void squad_leave_on_event(struct rr_ui_element *this,
                                 struct rr_game *game)
{
    if (game->socket_ready)
    {
        if (game->input_data->mouse_buttons_up_this_tick & 1)
        {
            game->socket_error = 0;
            struct proto_bug encoder;
            proto_bug_init(&encoder, RR_OUTGOING_PACKET);
            proto_bug_write_uint8(&encoder, game->socket.quick_verification, "qv");
            proto_bug_write_uint8(&encoder, rr_serverbound_squad_join, "header");
            proto_bug_write_uint8(&encoder, 3, "join type");
            rr_websocket_send(&game->socket, encoder.current - encoder.start);
        }
        rr_ui_render_tooltip_below(this, game->leave_squad_tooltip, game);
        game->cursor = rr_game_cursor_pointer;
    }
}

static uint8_t close_squad_button_should_show(struct rr_ui_element *this,
                                              struct rr_game *game)
{
    return game->socket_ready;
}

static struct rr_ui_element *close_squad_button_init(float w)
{
    struct rr_ui_element *this =
        rr_ui_close_button_init(w, squad_leave_on_event);
    this->should_show = close_squad_button_should_show;
    this->no_reposition = 1;
    rr_ui_pad(rr_ui_set_justify(this, 1, -1), 5);
    return this;
}

void rr_game_init(struct rr_game *this)
{
    memset(this, 0, sizeof *this);
    rr_static_data_init();
    this->window = rr_ui_container_init();
    this->window->container = this->window;
    this->window->h_justify = this->window->v_justify = 1;
    this->window->resizeable = 0;
    this->window->on_event = window_on_event;

    strcpy(this->rivet_account.name, "Guest");
    strcpy(this->rivet_account.token, "");
    strcpy(this->rivet_account.code, "");
    strcpy(this->rivet_account.uuid, "");
    rr_discord_oauth2_init(this);

    // clang-format off
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_h_container_init(rr_ui_container_init(), 10, 10,
                    rr_ui_settings_toggle_button_init(),
                    rr_ui_account_toggle_button_init(),
                    rr_ui_dev_panel_toggle_button_init(),
                    rr_ui_fullscreen_toggle_button_init(),
                    rr_ui_link_toggle(rr_ui_close_button_init(30, abandon_game_on_event), simulation_ready),
                    NULL
                ),
            -1, -1),
        ui_not_hidden)
    );

    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_h_container_init(rr_ui_container_init(), 10, 10,
                    rr_ui_gardn_toggle_button_init(),
                    rr_ui_github_toggle_button_init(),
                    rr_ui_discord_toggle_button_init(),
                    NULL
                ),
            1, -1),
        ui_not_hidden)
    );

    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
        rr_ui_set_background(
            rr_ui_pad(
                rr_ui_set_justify(
                    rr_ui_h_container_init(rr_ui_container_init(), 10, 10, 
                    rr_ui_minimap_init(this),
                    NULL
                )
            , 1, -1),
        10),
    0x80000000),
        ui_not_hidden_and_simulation_ready
        )
    );
    rr_ui_container_add_element(this->window, rr_ui_link_toggle(
        rr_ui_pad(
            rr_ui_v_pad(
                rr_ui_set_justify(
                    rr_ui_v_container_init(rr_ui_container_init(), 10, 40,
                        rr_ui_in_game_player_hud_init(0),
                        rr_ui_in_game_player_hud_init(1),
                        rr_ui_in_game_player_hud_init(2),
                        rr_ui_in_game_player_hud_init(3),
                        rr_ui_in_game_player_hud_init(4),
                        NULL
                    )
                , -1, -1)
            , 100)
        , 50)
    , ui_not_hidden_and_simulation_ready));
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_background(
                rr_ui_v_container_init(rr_ui_container_init(), 10, 20,
                    rr_ui_v_container_init(rr_ui_container_init(), 0, 10,
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10,
                            rr_ui_text_init("Rysteria", 96, 0xffffffff),
                            rr_ui_h_container_init(
                                rr_ui_container_init(), 0, 20,
                                rr_ui_link_toggle(
                                    rr_ui_set_fill_stroke(
                                        rr_ui_h_container_init(rr_ui_container_init(), 0, 0,
                                            rr_ui_text_input_init(350, 30, &this->cache.nickname[0], 16, "_0x4346"), 
                                            NULL
                                        ), 
                                    0x00000000, 0x00000000),
                                simulation_not_ready),
                                rr_ui_join_button_init(),
                                NULL
                            ),
                            /*
                            rr_ui_h_container_init(rr_ui_container_init(), 0, 10,
                                rr_ui_biome_button_init("Hell Creek", 0xffff0000, 0),
                                rr_ui_biome_button_init("Ocean", 0xffcdb423, 1),
                                NULL
                            ),
                            */
                            rr_ui_set_justify(
                                rr_ui_h_container_init(rr_ui_container_init(), 0, 10, 
                                rr_ui_create_squad_button_init(),
                                rr_ui_squad_button_init(),
                                NULL
                            ), 1, -1),
                            NULL
                        ),
                        rr_ui_set_background(
                            rr_ui_link_toggle(
                                rr_ui_container_add_element(
                                    rr_ui_v_container_init(
                                        rr_ui_popup_container_init(), 10, 10,
                                        rr_ui_text_init("Squad", 24, 0xffffffff),
                                        rr_ui_squad_toggle_buttons_container_init(this),
                                        rr_ui_multi_choose_element_init(
                                            socket_ready,
                                            rr_ui_text_init("Connecting...", 24, 0xffffffff),
                                            rr_ui_squad_container_init(&this->squad),
                                            rr_ui_text_init("Disconnected", 24, 0xffff2222),
                                            rr_ui_text_init("Failed to join squad", 24, 0xffff2222),
                                            rr_ui_text_init("Squad doesn't exist", 24, 0xffff2222),
                                            rr_ui_text_init("Squad is full", 24, 0xffff2222),
                                            rr_ui_text_init("Kicked from squad", 24, 0xffff2222),
                                            rr_ui_text_init("Kicked for AFK", 24, 0xffff2222),
                                            NULL
                                        ),
                                        rr_ui_flex_container_init(
                                            rr_ui_copy_squad_code_button_init(),
                                            rr_ui_h_container_init(rr_ui_container_init(), 0, 10,
                                                rr_ui_set_fill_stroke(
                                                    rr_ui_h_container_init(rr_ui_container_init(), 0, 0,
                                                        rr_ui_text_input_init(100, 18, &this->connect_code[0], 16, "_0x4347"), 
                                                        NULL
                                                    ), 
                                                0x00000000, 0x00000000),
                                                rr_ui_join_squad_code_button_init(),
                                                NULL
                                            ),
                                            10
                                        ),
                                        NULL
                                    ),
                                    close_squad_button_init(25)
                                )->container, 
                            socket_pending_or_ready
                        ), 0x40ffffff),
                        NULL
                    ),
                    rr_ui_level_bar_init(400),
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 15,
                        rr_ui_title_screen_loadout_button_init(0),
                        rr_ui_title_screen_loadout_button_init(1),
                        rr_ui_title_screen_loadout_button_init(2),
                        rr_ui_title_screen_loadout_button_init(3),
                        rr_ui_title_screen_loadout_button_init(4),
                        rr_ui_title_screen_loadout_button_init(5),
                        rr_ui_title_screen_loadout_button_init(6),
                        rr_ui_title_screen_loadout_button_init(7),
                        rr_ui_title_screen_loadout_button_init(8),
                        rr_ui_title_screen_loadout_button_init(9),
                        rr_ui_title_screen_loadout_button_init(10),
                        rr_ui_title_screen_loadout_button_init(11),
                        NULL
                    ),
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 15,
                        rr_ui_title_screen_loadout_button_init(12),
                        rr_ui_title_screen_loadout_button_init(13),
                        rr_ui_title_screen_loadout_button_init(14),
                        rr_ui_title_screen_loadout_button_init(15),
                        rr_ui_title_screen_loadout_button_init(16),
                        rr_ui_title_screen_loadout_button_init(17),
                        rr_ui_title_screen_loadout_button_init(18),
                        rr_ui_title_screen_loadout_button_init(19),
                        rr_ui_title_screen_loadout_button_init(20),
                        rr_ui_title_screen_loadout_button_init(21),
                        rr_ui_title_screen_loadout_button_init(22),
                        rr_ui_title_screen_loadout_button_init(23),
                        NULL
                    ),
                    // rr_ui_text_init("https://github.com/maxnest0x0/rysteria", 15, 0xffffffff),
                    NULL
                ),
            0x00000000),
        simulation_not_ready)
    );
    
    rr_ui_container_add_element(this->window, rr_ui_finished_game_screen_init());
    rr_ui_container_add_element(this->window, rr_ui_loot_container_init());

    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_v_container_init(rr_ui_container_init(), 15, 15,
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15,
                        rr_ui_text_init("[X]", 18, 0xffffffff),
                        rr_ui_loadout_button_init(0),
                        rr_ui_loadout_button_init(1),
                        rr_ui_loadout_button_init(2),
                        rr_ui_loadout_button_init(3),
                        rr_ui_loadout_button_init(4),
                        rr_ui_loadout_button_init(5),
                        rr_ui_loadout_button_init(6),
                        rr_ui_loadout_button_init(7),
                        rr_ui_loadout_button_init(8),
                        rr_ui_loadout_button_init(9),
                        rr_ui_loadout_button_init(10),
                        rr_ui_loadout_button_init(11),
                        rr_ui_text_init("[X]", 18, 0x00000000),
                        NULL
                    ),
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15,
                        rr_ui_secondary_loadout_button_init(0),
                        rr_ui_secondary_loadout_button_init(1),
                        rr_ui_secondary_loadout_button_init(2),
                        rr_ui_secondary_loadout_button_init(3),
                        rr_ui_secondary_loadout_button_init(4),
                        rr_ui_secondary_loadout_button_init(5),
                        rr_ui_secondary_loadout_button_init(6),
                        rr_ui_secondary_loadout_button_init(7),
                        rr_ui_secondary_loadout_button_init(8),
                        rr_ui_secondary_loadout_button_init(9),
                        rr_ui_secondary_loadout_button_init(10),
                        rr_ui_secondary_loadout_button_init(11),
                        NULL
                    ),
                    NULL
                ),
            0, 1),
        ui_not_hidden_and_simulation_ready)
    );

    rr_ui_container_add_element(
        this->window,
        rr_ui_set_justify(
            rr_ui_h_container_init(rr_ui_container_init(), 0, 0,
                rr_ui_link_toggle(
                    rr_ui_set_justify(
                        rr_ui_v_container_init(rr_ui_container_init(), 10, 10,
                            rr_ui_inventory_toggle_button_init(),
                            rr_ui_mob_gallery_toggle_button_init(),
                            rr_ui_crafting_toggle_button_init(),
                            NULL
                        ),
                    -1, 1),
                ui_not_hidden_and_player_dead),
                rr_ui_pad(
                    rr_ui_set_justify(
                        rr_ui_chat_bar_init(this),
                    -1, 1)
                , 20),
                NULL
            ),
        -1, 1)
    );
    this->window->elements.start[this->window->elements.size - 1]
        ->pass_on_event = 1;

    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_inventory_container_init(), close_menu_button_init(25))->container);
    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_mob_container_init(), close_menu_button_init(25))->container);
    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_crafting_container_init(this), close_menu_button_init(25))->container);
    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_settings_container_init(this), close_menu_button_init(25))->container);
    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_account_container_init(this), close_menu_button_init(25))->container);
    rr_ui_container_add_element(this->window, rr_ui_container_add_element(rr_ui_dev_panel_container_init(this), close_menu_button_init(25))->container);

    this->link_account_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Login with Discord", 16)
    );

    this->inventory_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Inventory", 16)
    );

    this->gallery_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Mob Gallery", 16)
    );

    this->craft_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Crafting", 16)
    );

    this->settings_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Settings", 16)
    );

    this->abandon_game_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Leave Game", 16)
    );

    this->account_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Account", 16)
    );

    this->squads_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Squads", 16)
    );

    this->discord_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Join our Discord!", 16)
    );

    this->github_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("We're open source!", 16)
    );

    this->gardn_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Try the gardn project!", 16)
    );

    this->fullscreen_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Fullscreen", 16)
    );

    this->link_reminder_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Login to save progress across devices", 16)
    );

    this->leave_squad_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Leave Squad", 14)
    );

    this->click_to_copy_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Click to copy", 14)
    );

    this->kick_from_squad_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Kick from squad", 12)
    );

    this->vote_for_kick_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Vote for kick", 12)
    );

    this->block_in_chat_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Block in chat", 12)
    );

    this->unblock_in_chat_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Unblock in chat", 12)
    );

    this->transfer_ownership_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Transfer ownership", 12)
    );

    this->squad_owner_tooltip = rr_ui_container_add_element(
        this->window,
        make_label_tooltip("Squad owner", 12)
    );

    this->anti_afk = rr_ui_container_add_element(
        this->window,
        rr_ui_anti_afk_container_init(this)
    );

    // clang-format on

    for (uint32_t i = 0; i < RR_SQUAD_MEMBER_COUNT; ++i)
    {
        this->squad.squad_members[i].tooltip =
            rr_ui_squad_player_tooltip_init(&this->squad.squad_members[i]);
        rr_ui_container_add_element(this->window,
                                    this->squad.squad_members[i].tooltip);
    }
    for (uint32_t i = 0; i < RR_SQUAD_COUNT; ++i)
        for (uint32_t j = 0; j < RR_SQUAD_MEMBER_COUNT; ++j)
        {
            struct rr_squad_member *member =
                &this->other_squads[i].squad_members[j];
            member->tooltip = rr_ui_squad_player_tooltip_init(member);
            rr_ui_container_add_element(this->window, member->tooltip);
        }

    for (uint32_t id = 0; id < rr_mob_id_max; ++id)
    {
        for (uint32_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
        {
            this->mob_tooltips[id][rarity] = rr_ui_mob_tooltip_init(id, rarity);
            rr_ui_container_add_element(this->window,
                                        this->mob_tooltips[id][rarity]);
        }
    }
    for (uint32_t id = 0; id < rr_petal_id_max; ++id)
    {
        for (uint32_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
        {
            this->petal_tooltips[id][rarity] =
                rr_ui_petal_tooltip_init(id, rarity);
            rr_ui_container_add_element(this->window,
                                        this->petal_tooltips[id][rarity]);
            // remember that these don't have a container
        }
    }

    rr_assets_init(this);
    rr_game_cache_load(this);
    rr_dom_set_text("_0x4346", this->cache.nickname);
    this->ticks_until_text_cache = 24;
    this->is_mobile = rr_dom_test_mobile();
    this->slots_unlocked =
        RR_SLOT_COUNT_FROM_LEVEL(level_from_xp(this->cache.experience));
}

void rr_game_websocket_on_event_function(enum rr_websocket_event_type type,
                                         void *data, void *captures,
                                         uint64_t size)
{
    struct rr_game *this = captures;
    switch (type)
    {
    case rr_websocket_event_type_open:
        puts("<rr_websocket::open>");
        this->socket_error = 0;
        break;
    case rr_websocket_event_type_close:
        // memset the clients
        printf("<rr_websocket::close::%llu>\n", size);
        this->socket_ready = 0;
        this->socket_pending = 0;
        this->socket_error = 1;
        if (this->simulation_ready)
        {
            rr_simulation_init(this->simulation);
            rr_particle_manager_clear(&this->default_particle_manager);
            rr_particle_manager_clear(&this->foreground_particle_manager);
        }
        this->simulation_ready = 0;
        this->socket.recieved_first_packet = 0;
        break;
    case rr_websocket_event_type_data:
    {
        struct proto_bug encoder;
        proto_bug_init(&encoder, data);
        if (!this->socket.recieved_first_packet)
        {
            this->socket.recieved_first_packet = 1;
            rr_decrypt(data, 1024, 21094093777837637ull);
            rr_decrypt(data, 8, 1);
            rr_decrypt(data, 1024, 59731158950470853ull);
            rr_decrypt(data, 1024, 64709235936361169ull);
            rr_decrypt(data, 1024, 59013169977270713ull);
            uint64_t verification =
                proto_bug_read_uint64(&encoder, "verification");
            proto_bug_read_uint32(&encoder, "useless bytes");
            this->socket.clientbound_encryption_key =
                proto_bug_read_uint64(&encoder, "c encryption key");
            this->socket.serverbound_encryption_key =
                proto_bug_read_uint64(&encoder, "s encryption key");
            this->socket.quick_verification = RR_SECRET8;
            // respond
            struct proto_bug verify_encoder;
            proto_bug_init(&verify_encoder, RR_OUTGOING_PACKET);
            proto_bug_write_uint64(&verify_encoder, rr_get_rand(),
                                   "useless bytes");
            proto_bug_write_uint64(&verify_encoder, verification,
                                   "verification");
            proto_bug_write_string(&verify_encoder, this->rivet_account.token,
                                   300, "rivet token");
            proto_bug_write_string(&verify_encoder, this->rivet_account.uuid,
                                   100, "rivet uuid");
            proto_bug_write_string(&verify_encoder, this->rivet_account.code,
                                   100, "oauth2 code");
            proto_bug_write_varuint(&verify_encoder, this->dev_flag,
                                    "dev_flag");
            rr_websocket_send(&this->socket,
                              verify_encoder.current - verify_encoder.start);
            return;
        }
        this->socket_ready =
            1; // signifies that the socket is verified on the serverside
        this->socket_pending = 0;
        // send instajoin
        this->socket.clientbound_encryption_key =
            rr_get_hash(this->socket.clientbound_encryption_key);
        rr_decrypt(data, size, this->socket.clientbound_encryption_key);
        uint8_t h = proto_bug_read_uint8(&encoder, "header");
        switch (h)
        {
        case rr_clientbound_update:
        {
            this->socket_error = 0;
            this->joined_squad = 1;

            this->kick_vote_pos = proto_bug_read_uint8(&encoder, "kick vote");
            for (uint32_t i = 0; i < RR_SQUAD_MEMBER_COUNT; ++i)
            {
                this->squad.squad_members[i].in_use =
                    proto_bug_read_uint8(&encoder, "bitbit");
                if (this->squad.squad_members[i].in_use == 0)
                    continue;
                this->squad.squad_members[i].playing =
                    proto_bug_read_uint8(&encoder, "ready");
                this->squad.squad_members[i].disconnected =
                    proto_bug_read_uint8(&encoder, "disconnected");
                this->squad.squad_members[i].blocked =
                    proto_bug_read_uint8(&encoder, "blocked");
                this->squad.squad_members[i].is_dev =
                    proto_bug_read_uint8(&encoder, "is_dev");
                uint8_t kick_vote_count =
                    proto_bug_read_uint8(&encoder, "kick votes");
                if (this->squad.squad_members[i].kick_vote_count !=
                    kick_vote_count)
                {
                    this->squad.squad_members[i].kick_vote_count =
                        kick_vote_count;
                    sprintf(this->squad.squad_members[i].kick_text, "%u/%u",
                            kick_vote_count, RR_SQUAD_MEMBER_COUNT - 1);
                    if (this->kick_vote_pos == i)
                        rr_ui_set_background(
                            this->squad.squad_members[i].kick_text_el,
                            0xffff4444);
                    else
                        rr_ui_set_background(
                            this->squad.squad_members[i].kick_text_el,
                            0xffffffff);
                }
                uint32_t level = proto_bug_read_varuint(&encoder, "level");
                if (this->squad.squad_members[i].level != level)
                {
                    float health = 200 * pow(1.0256, level - 1);
                    float damage = 0.1 * health;
                    this->squad.squad_members[i].level = level;
                    sprintf(this->squad.squad_members[i].level_text,
                            "%u", level);
                    rr_sprintf(this->squad.squad_members[i].health_text,
                               health);
                    rr_sprintf(this->squad.squad_members[i].damage_text,
                               damage);
                }
                proto_bug_read_string(&encoder,
                                      this->squad.squad_members[i].nickname, 16,
                                      "nickname");
                proto_bug_read_string(&encoder,
                                      this->squad.squad_members[i].uuid, 37,
                                      "uuid");
                proto_bug_read_string(&encoder,
                                      this->squad.squad_members[i].discord, 20,
                                      "discord");
                for (uint32_t j = 0; j < RR_MAX_SLOT_COUNT * 2; ++j)
                {
                    this->squad.squad_members[i].loadout[j].id =
                        proto_bug_read_uint8(&encoder, "id");
                    this->squad.squad_members[i].loadout[j].rarity =
                        proto_bug_read_uint8(&encoder, "rar");
                }
            }
            this->squad.squad_index = proto_bug_read_uint8(&encoder, "sqidx");
            this->squad.squad_owner = proto_bug_read_uint8(&encoder, "sqown");
            this->squad.squad_pos = proto_bug_read_uint8(&encoder, "sqpos");
            this->squad.squad_private =
                proto_bug_read_uint8(&encoder, "private");
            this->squad.squad_expose_code =
                proto_bug_read_uint8(&encoder, "expose_code");
            this->selected_biome = proto_bug_read_uint8(&encoder, "biome");
            proto_bug_read_string(&encoder, this->squad.squad_code, 16,
                                  "squad code");
            // this->is_dev =
            //     this->squad.squad_members[this->squad.squad_pos].is_dev;
            this->afk = proto_bug_read_uint8(&encoder, "afk");
            if (this->afk)
                proto_bug_read_string(&encoder, this->afk_challenge, 7,
                                      "afk_challenge");
            if (proto_bug_read_uint8(&encoder, "in game") == 1)
            {
                if (!this->simulation_ready)
                {
                    rr_simulation_init(this->simulation);
                    rr_simulation_init(this->deletion_simulation);
                    rr_particle_manager_clear(&this->default_particle_manager);
                    rr_particle_manager_clear(
                        &this->foreground_particle_manager);
                    rr_write_dev_cheat_packets(this, 1);
                    this->simulation_ready = 1;
                }
                rr_simulation_read_binary(this, &encoder);
            }
            else
            {
                if (this->simulation_ready)
                {
                    rr_simulation_init(this->simulation);
                    rr_particle_manager_clear(&this->default_particle_manager);
                    rr_particle_manager_clear(
                        &this->foreground_particle_manager);
                }
                this->simulation_ready = 0;
                proto_bug_init(&encoder, RR_OUTGOING_PACKET);
                proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
                proto_bug_write_uint8(&encoder, rr_serverbound_squad_update,
                                      "header");
                proto_bug_write_string(&encoder, this->cache.nickname, 16,
                                       "nickname");
                proto_bug_write_uint8(&encoder, this->slots_unlocked,
                                      "loadout count");
                for (uint32_t i = 0; i < this->slots_unlocked; ++i)
                {
                    proto_bug_write_uint8(&encoder, this->cache.loadout[i].id,
                                          "id");
                    proto_bug_write_uint8(
                        &encoder, this->cache.loadout[i].rarity, "rarity");
                    proto_bug_write_uint8(
                        &encoder,
                        this->cache.loadout[i + RR_MAX_SLOT_COUNT].id, "id");
                    proto_bug_write_uint8(
                        &encoder,
                        this->cache.loadout[i + RR_MAX_SLOT_COUNT].rarity,
                        "rarity");
                }
                rr_websocket_send(&this->socket,
                                  encoder.current - encoder.start);
            }
            break;
        }
        case rr_clientbound_squad_dump:
        {
            this->is_dev = proto_bug_read_uint8(&encoder, "is_dev");
            this->kick_vote_pos = proto_bug_read_uint8(&encoder, "kick vote");
            for (uint32_t s = 0; s < RR_SQUAD_COUNT; ++s)
            {
                struct rr_game_squad *squad = &this->other_squads[s];
                for (uint32_t i = 0; i < RR_SQUAD_MEMBER_COUNT; ++i)
                {
                    squad->squad_members[i].in_use =
                        proto_bug_read_uint8(&encoder, "bitbit");
                    if (squad->squad_members[i].in_use == 0)
                        continue;
                    squad->squad_members[i].playing =
                        proto_bug_read_uint8(&encoder, "ready");
                    squad->squad_members[i].disconnected =
                        proto_bug_read_uint8(&encoder, "disconnected");
                    squad->squad_members[i].blocked =
                        proto_bug_read_uint8(&encoder, "blocked");
                    squad->squad_members[i].is_dev =
                        proto_bug_read_uint8(&encoder, "is_dev");
                    uint8_t kick_vote_count =
                        proto_bug_read_uint8(&encoder, "kick votes");
                    if (squad->squad_members[i].kick_vote_count !=
                        kick_vote_count)
                    {
                        squad->squad_members[i].kick_vote_count =
                            kick_vote_count;
                        sprintf(squad->squad_members[i].kick_text, "%u/%u",
                                kick_vote_count, RR_SQUAD_MEMBER_COUNT - 1);
                        if (this->joined_squad &&
                            this->squad.squad_index == s &&
                            this->kick_vote_pos == i)
                            rr_ui_set_background(
                                squad->squad_members[i].kick_text_el,
                                0xffff4444);
                        else
                            rr_ui_set_background(
                                squad->squad_members[i].kick_text_el,
                                0xffffffff);
                    }
                    uint32_t level = proto_bug_read_varuint(&encoder, "level");
                    if (squad->squad_members[i].level != level)
                    {
                        float health = 200 * pow(1.0256, level - 1);
                        float damage = 0.1 * health;
                        squad->squad_members[i].level = level;
                        sprintf(squad->squad_members[i].level_text,
                                "%u", level);
                        rr_sprintf(squad->squad_members[i].health_text, health);
                        rr_sprintf(squad->squad_members[i].damage_text, damage);
                    }
                    proto_bug_read_string(&encoder,
                                          squad->squad_members[i].nickname, 16,
                                          "nickname");
                    proto_bug_read_string(&encoder,
                                          squad->squad_members[i].uuid, 37,
                                          "uuid");
                    proto_bug_read_string(&encoder,
                                          squad->squad_members[i].discord, 20,
                                          "discord");
                    for (uint32_t j = 0; j < RR_MAX_SLOT_COUNT * 2; ++j)
                    {
                        squad->squad_members[i].loadout[j].id =
                            proto_bug_read_uint8(&encoder, "id");
                        squad->squad_members[i].loadout[j].rarity =
                            proto_bug_read_uint8(&encoder, "rar");
                    }
                }
                squad->squad_index = s;
                squad->squad_owner = proto_bug_read_uint8(&encoder, "sqown");
                squad->squad_private =
                    proto_bug_read_uint8(&encoder, "private");
                squad->squad_expose_code =
                    proto_bug_read_uint8(&encoder, "expose_code");
                this->selected_biome = proto_bug_read_uint8(&encoder, "biome");
                proto_bug_read_string(&encoder, squad->squad_code, 16,
                                      "squad code");
            }
            break;
        }
        case rr_clientbound_animation_update:
        {
            while (proto_bug_read_uint8(&encoder, "continue"))
            {
                struct rr_simulation_animation *particle;
                uint8_t type = proto_bug_read_uint8(&encoder, "ani type");
                if (type != rr_animation_type_chat)
                    particle =
                        rr_particle_alloc(&this->foreground_particle_manager,
                                          type);
                switch (type)
                {
                case rr_animation_type_lightningbolt:
                    particle->length =
                        proto_bug_read_uint8(&encoder, "ani length");
                    for (uint32_t i = 0; i < particle->length; ++i)
                    {
                        particle->points[i].x =
                            proto_bug_read_float32(&encoder, "ani x");
                        particle->points[i].y =
                            proto_bug_read_float32(&encoder, "ani y");
                    }
                    particle->opacity = 0.8;
                    particle->disappearance = 6;
                    break;
                case rr_animation_type_damagenumber:
                {
                    particle->x = proto_bug_read_float32(&encoder, "ani x");
                    particle->y = proto_bug_read_float32(&encoder, "ani y");
                    particle->velocity.x = (rr_frand() - 0.5) * 25;
                    particle->velocity.y = -15 + rr_frand() * 5;
                    particle->acceleration.y = 0.75;
                    particle->friction = 0.9;
                    particle->damage =
                        proto_bug_read_varuint(&encoder, "damage");
                    switch (proto_bug_read_uint8(&encoder, "color type"))
                    {
                    case rr_animation_color_type_damage:
                        particle->color = 0xffff4444;
                        break;
                    case rr_animation_color_type_heal:
                        particle->color = 0xffffff44;
                        break;
                    case rr_animation_color_type_uranium:
                        particle->color = 0xff63bf2e;
                        break;
                    case rr_animation_color_type_fireball:
                        particle->color = 0xffce5d0b;
                        break;
                    case rr_animation_color_type_lightning:
                        particle->color = 0xffccccfc;
                        break;
                    }
                    particle->opacity = 1;
                    particle->disappearance = 6;
                    break;
                }
                case rr_animation_type_chat:
                    if (this->chat.at < 9)
                        this->chat.at++;
                    else
                    {
                        for (uint8_t i = 0; i < 9; i++)
                            this->chat.messages[i] = this->chat.messages[i + 1];
                    }
                    struct rr_game_chat_message *message = &this->chat.messages[this->chat.at];
                    proto_bug_read_string(&encoder, message->sender_name, 64, "name");
                    proto_bug_read_string(&encoder, message->message, 64, "chat");
                    sprintf(message->text, "%s: %s", message->sender_name, message->message);
                    break;
                case rr_animation_type_area_damage:
                    particle->x = proto_bug_read_float32(&encoder, "ani x");
                    particle->y = proto_bug_read_float32(&encoder, "ani y");
                    particle->size = proto_bug_read_float32(&encoder, "size");
                    switch (proto_bug_read_uint8(&encoder, "color type"))
                    {
                    case rr_animation_color_type_uranium:
                        particle->color = 0x2063bf2e;
                        break;
                    case rr_animation_color_type_fireball:
                        particle->color = 0x80ce5d0b;
                        break;
                    }
                    particle->opacity = 1;
                    particle->disappearance = 10 * sqrtf(500 / particle->size);
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case rr_clientbound_squad_fail:
            this->socket_error =
                3 + proto_bug_read_uint8(&encoder, "fail type");
            if (this->simulation_ready)
            {
                rr_simulation_init(this->simulation);
                rr_particle_manager_clear(&this->default_particle_manager);
                rr_particle_manager_clear(&this->foreground_particle_manager);
            }
            this->simulation_ready = 0;
            this->joined_squad = 0;
            break;
        case rr_clientbound_squad_leave:
            this->joined_squad = 0;
            break;
        case rr_clientbound_account_result:
            rr_game_read_account(this, &encoder);
            break;
        case rr_clientbound_craft_result:
        {
            this->crafting_data.crafting_id =
                proto_bug_read_uint8(&encoder, "craft id");
            this->crafting_data.crafting_rarity =
                proto_bug_read_uint8(&encoder, "craft rarity");
            this->crafting_data.temp_successes =
                proto_bug_read_varuint(&encoder, "success count");
            this->crafting_data.temp_fails =
                proto_bug_read_varuint(&encoder, "fail count");
            this->crafting_data.temp_attempts =
                proto_bug_read_varuint(&encoder, "attempts");
            this->crafting_data.last_attempts =
                proto_bug_read_varuint(&encoder, "last attempts");
            this->crafting_data.temp_xp =
                proto_bug_read_float64(&encoder, "craft xp");
            this->crafting_data.animation =
                powf(1.25, this->crafting_data.crafting_rarity);
            if (this->crafting_data.temp_successes == 0)
                this->crafting_data.animation *=
                    (5 - (this->crafting_data.count -
                          this->crafting_data.temp_fails)) / 5.0f;
            break;
        }
        case rr_clientbound_oauth2_data:
            rr_discord_oauth2_read_data(this, &encoder);
            break;
        default:
            RR_UNREACHABLE("how'd this happen");
        }
        break;
    }
    default:
        RR_UNREACHABLE("non exhaustive switch expression");
    }
}

void render_drop_component(EntityIdx entity, struct rr_game *this,
                           struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_drop_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_health_component(EntityIdx entity, struct rr_game *this,
                             struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x,
                          physical->lerp_y + physical->radius + 30);
    rr_component_health_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_mob_component(EntityIdx entity, struct rr_game *this,
                          struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state1;
    struct rr_renderer_context_state state2;
    rr_renderer_context_state_init(this->renderer, &state1);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_renderer_context_state_init(this->renderer, &state2);
    rr_component_mob_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state2);
    if (this->cache.show_hitboxes)
    {
        struct rr_component_mob *mob =
            rr_simulation_get_mob(simulation, entity);
        rr_renderer_begin_path(this->renderer);
        rr_renderer_set_stroke(this->renderer, RR_RARITY_COLORS[mob->rarity]);
        rr_renderer_set_line_width(this->renderer, 2);
        rr_renderer_set_global_alpha(this->renderer,
                                     1 - physical->deletion_animation);
        rr_renderer_arc(this->renderer, 0, 0, physical->radius);
        rr_renderer_stroke(this->renderer);
    }
    rr_renderer_context_state_free(this->renderer, &state1);
}

void render_petal_component(EntityIdx entity, struct rr_game *this,
                            struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state1;
    struct rr_renderer_context_state state2;
    rr_renderer_context_state_init(this->renderer, &state1);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_renderer_context_state_init(this->renderer, &state2);
    rr_component_petal_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state2);
    if (this->cache.show_hitboxes)
    {
        struct rr_component_petal *petal =
            rr_simulation_get_petal(simulation, entity);
        rr_renderer_begin_path(this->renderer);
        rr_renderer_set_stroke(this->renderer, RR_RARITY_COLORS[petal->rarity]);
        rr_renderer_set_line_width(this->renderer, 2);
        rr_renderer_set_global_alpha(this->renderer,
                                     1 - physical->deletion_animation);
        rr_renderer_arc(this->renderer, 0, 0, physical->radius);
        rr_renderer_stroke(this->renderer);
    }
    rr_renderer_context_state_free(this->renderer, &state1);
}

void render_flower_component(EntityIdx entity, struct rr_game *this,
                             struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_flower_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_web_component(EntityIdx entity, struct rr_game *this,
                          struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_web_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_nest_component(EntityIdx entity, struct rr_game *this,
                           struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_nest_render(entity, this, simulation);
    rr_renderer_context_state_free(this->renderer, &state);
}

void player_info_finder(struct rr_game *this)
{
    struct rr_simulation *simulation = this->simulation;
    uint8_t counter = 1;
    memset(&this->player_infos, 0, sizeof this->player_infos);
    this->player_infos[0] = this->player_info->parent_id;
    for (EntityIdx i = 0; i < simulation->player_info_count; ++i)
        if (simulation->player_info_vector[i] != this->player_info->parent_id)
            this->player_infos[counter++] = simulation->player_info_vector[i];
}

static void write_serverbound_packet_desktop(struct rr_game *this)
{
    struct proto_bug encoder2;
    proto_bug_init(&encoder2, RR_OUTGOING_PACKET);
    proto_bug_write_uint8(&encoder2, this->socket.quick_verification, "qv");
    proto_bug_write_uint8(&encoder2, rr_serverbound_input, "header");
    uint8_t movement_flags = 0;
    if (!this->chat.chat_active)
    {
        movement_flags |= (rr_bitset_get(this->input_data->keys_pressed, 'W') ||
                        rr_bitset_get(this->input_data->keys_pressed, 38))
                        << 0;
        movement_flags |= (rr_bitset_get(this->input_data->keys_pressed, 'A') ||
                        rr_bitset_get(this->input_data->keys_pressed, 37))
                        << 1;
        movement_flags |= (rr_bitset_get(this->input_data->keys_pressed, 'S') ||
                        rr_bitset_get(this->input_data->keys_pressed, 40))
                        << 2;
        movement_flags |= (rr_bitset_get(this->input_data->keys_pressed, 'D') ||
                        rr_bitset_get(this->input_data->keys_pressed, 39))
                        << 3;
    }
    movement_flags |= this->input_data->mouse_buttons << 4;
    if (!this->chat.chat_active)
    {
        movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 32) << 4;
        movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 16) << 5;
    }
    if (!(movement_flags >> 4 & 3))
    {
        movement_flags |= this->cache.hold_attack << 4;
        movement_flags |= this->cache.hold_defense << 5;
    }
    movement_flags |= this->cache.use_mouse << 6;

    proto_bug_write_uint8(&encoder2, movement_flags, "movement kb flags");
    if (this->cache.use_mouse)
    {
        proto_bug_write_float32(
            &encoder2, this->input_data->mouse_x - this->renderer->width / 2,
            "mouse x");
        proto_bug_write_float32(
            &encoder2, this->input_data->mouse_y - this->renderer->height / 2,
            "mouse y");
    }
    rr_websocket_send(&this->socket, encoder2.current - encoder2.start);

    struct proto_bug encoder;
    proto_bug_init(&encoder, RR_OUTGOING_PACKET);
    proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
    proto_bug_write_uint8(&encoder, rr_serverbound_petal_switch, "header");
    if (!this->chat.chat_active)
    {
        uint8_t should_write = 0;
        uint8_t switch_all =
            rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'X');
        for (uint8_t n = 1; n <= this->slots_unlocked; ++n)
        {
            uint32_t key = '0' + (n % 10);
            if (n == 11)
                key = 189;
            if (n == 12)
                key = 187;
            if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                                  key) || switch_all)
            {
                proto_bug_write_uint8(&encoder, n, "petal switch");
                should_write = 1;
            }
        }
        if (should_write)
        {
            proto_bug_write_uint8(&encoder, 0, "petal switch");
            rr_websocket_send(&this->socket, encoder.current - encoder.start);
        }
    }
}

void rr_write_dev_cheat_packets(struct rr_game *this, uint8_t force)
{
    if (!this->is_dev)
        return;
    uint8_t cheat_flags = 0;
    cheat_flags |= this->dev_cheats.invisible << 0;
    cheat_flags |= this->dev_cheats.invulnerable << 1;
    cheat_flags |= this->dev_cheats.no_aggro << 2;
    cheat_flags |= this->dev_cheats.no_wall_collision << 3;
    cheat_flags |= this->dev_cheats.no_collision << 4;
    cheat_flags |= this->dev_cheats.no_grid_influence << 5;
    if (force || cheat_flags != this->dev_cheats.flags_last_tick)
    {
        this->dev_cheats.flags_last_tick = cheat_flags;
        struct proto_bug encoder;
        proto_bug_init(&encoder, RR_OUTGOING_PACKET);
        proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
        proto_bug_write_uint8(&encoder, rr_serverbound_dev_cheat, "header");
        proto_bug_write_uint8(&encoder, rr_dev_cheat_flags, "cheat type");
        proto_bug_write_uint8(&encoder, cheat_flags, "cheat flags");
        rr_websocket_send(&this->socket, encoder.current - encoder.start);
    }
    if (force || this->dev_cheats.speed_percent !=
        this->dev_cheats.speed_percent_last_tick)
    {
        this->dev_cheats.speed_percent_last_tick =
            this->dev_cheats.speed_percent;
        struct proto_bug encoder;
        proto_bug_init(&encoder, RR_OUTGOING_PACKET);
        proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
        proto_bug_write_uint8(&encoder, rr_serverbound_dev_cheat, "header");
        proto_bug_write_uint8(&encoder, rr_dev_cheat_speed_percent, "cheat type");
        proto_bug_write_float32(&encoder, this->dev_cheats.speed_percent,
                                "speed percent");
        rr_websocket_send(&this->socket, encoder.current - encoder.start);
    }
    if (force || this->dev_cheats.fov_percent !=
        this->dev_cheats.fov_percent_last_tick)
    {
        this->dev_cheats.fov_percent_last_tick =
            this->dev_cheats.fov_percent;
        struct proto_bug encoder;
        proto_bug_init(&encoder, RR_OUTGOING_PACKET);
        proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
        proto_bug_write_uint8(&encoder, rr_serverbound_dev_cheat, "header");
        proto_bug_write_uint8(&encoder, rr_dev_cheat_fov_percent, "cheat type");
        proto_bug_write_float32(&encoder, this->dev_cheats.fov_percent,
                                "fov percent");
        rr_websocket_send(&this->socket, encoder.current - encoder.start);
    }
}

void rr_game_tick(struct rr_game *this, float delta)
{
    if (this->ticks_until_text_cache == 0)
    {
        rr_renderer_text_cache_redraw(NULL);
        this->ticks_until_text_cache = 255;
    }
    else if (this->ticks_until_text_cache < 25)
        --this->ticks_until_text_cache;
    this->lerp_delta = delta;
    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    this->text_input_focused = rr_is_text_input_focused();
    this->slots_unlocked =
        RR_SLOT_COUNT_FROM_LEVEL(level_from_xp(this->cache.experience));
    rr_game_validate_loadout(this);
    rr_game_update_significant_rarity(this);

    rr_game_cache_data(this);

    double time = start.tv_sec * 1000000 + start.tv_usec;
    rr_renderer_set_transform(this->renderer, 1, 0, 0, 0, 1, 0);
    rr_renderer_set_global_alpha(this->renderer, 1);
    struct rr_renderer_context_state grand_state;
    rr_renderer_context_state_init(this->renderer, &grand_state);

    if (this->simulation_ready)
    {
        rr_simulation_tick(this->simulation, this->lerp_delta);
        rr_deletion_simulation_tick(this->deletion_simulation,
                                    this->lerp_delta);

        this->renderer->state.filter.amount = 0;
        struct rr_renderer_context_state state1;
        struct rr_renderer_context_state state2;
        if (this->player_info != NULL)
        {
            player_info_finder(this);
            // screen shake
            rr_renderer_context_state_init(this->renderer, &state1);
            struct rr_component_player_info *player_info = this->player_info;
            rr_renderer_translate(this->renderer, this->renderer->width / 2,
                                  this->renderer->height / 2);
            rr_renderer_scale(this->renderer, player_info->lerp_camera_fov *
                                                  this->renderer->scale);
            rr_renderer_translate(this->renderer, -player_info->lerp_camera_x,
                                  -player_info->lerp_camera_y);

            if (this->cache.screen_shake &&
                player_info->flower_id != RR_NULL_ENTITY)
            {
                if (rr_simulation_get_health(this->simulation,
                    player_info->flower_id)->damage_animation > 0.25)
                {
                    float r = rr_frand() * 5;
                    float a = rr_frand() * 2 * M_PI;
                    rr_renderer_translate(this->renderer, r * cosf(a),
                                          r * sinf(a));
                }
            }
            rr_component_arena_render(player_info->arena, this,
                                      this->simulation);

#define render_component(COMPONENT, filter)                                    \
    for (uint32_t i = 0; i < this->simulation->COMPONENT##_count; ++i)         \
        if (filter(this->simulation, this->simulation->COMPONENT##_vector[i])) \
            render_##COMPONENT##_component(                                    \
                this->simulation->COMPONENT##_vector[i], this,                 \
                this->simulation);                                             \
    for (uint32_t i = 0; i < this->deletion_simulation->COMPONENT##_count;     \
         ++i)                                                                  \
        if (filter(this->deletion_simulation,                                  \
                   this->deletion_simulation->COMPONENT##_vector[i]))          \
            render_##COMPONENT##_component(                                    \
                this->deletion_simulation->COMPONENT##_vector[i], this,        \
                this->deletion_simulation);

#define no_filter(simulation, id) 1
#define dead_flower_filter(simulation, id)                                     \
    rr_simulation_get_flower(simulation, id)->dead
#define alive_flower_filter(simulation, id)                                    \
    rr_simulation_get_flower(simulation, id)->dead == 0

            render_component(nest, no_filter);
            render_component(web, no_filter);
            render_component(health, no_filter);
            render_component(flower, dead_flower_filter);
            render_component(drop, no_filter);
            render_component(mob, no_filter);
            rr_system_particle_render_tick(
                this, &this->default_particle_manager, delta);
            render_component(petal, no_filter);
            render_component(flower, alive_flower_filter);
            rr_system_particle_render_tick(
                this, &this->foreground_particle_manager, delta);
            rr_renderer_context_state_free(this->renderer, &state1);
#undef render_component
#undef no_filter
#undef dead_flower_filter
#undef alive_flower_filter
        }
    }
    else
    {
        this->flower_dead = 1;
        struct rr_renderer_context_state state1;
        rr_renderer_context_state_init(this->renderer, &state1);
        rr_renderer_translate(this->renderer, this->renderer->width / 2,
                              this->renderer->height / 2);
        rr_renderer_scale(this->renderer, 1 * this->renderer->scale);
        rr_renderer_translate(this->renderer, -0, -0);
        double scale = 1 * this->renderer->scale;
        double leftX = 0 - this->renderer->width / (2 * scale);
        double rightX = 0 + this->renderer->width / (2 * scale);
        double topY = 0 - this->renderer->height / (2 * scale);
        double bottomY = 0 + this->renderer->height / (2 * scale);

#define GRID_SIZE (512.0f)
        double newLeftX = floorf(leftX / GRID_SIZE) * GRID_SIZE;
        double newTopY = floorf(topY / GRID_SIZE) * GRID_SIZE;
        for (; newLeftX < rightX; newLeftX += GRID_SIZE)
        {
            for (double currY = newTopY; currY < bottomY; currY += GRID_SIZE)
            {
                uint32_t tile_index =
                    rr_get_hash((uint32_t)(((newLeftX + 8192) / GRID_SIZE + 1) *
                                           ((currY + 8192) / GRID_SIZE + 2))) %
                    3;
                struct rr_renderer_context_state state;
                rr_renderer_context_state_init(this->renderer, &state);
                rr_renderer_translate(this->renderer, newLeftX + GRID_SIZE / 2,
                                      currY + GRID_SIZE / 2);
                rr_renderer_scale(this->renderer, (GRID_SIZE + 2) / 256);
                if (this->selected_biome == 0)
                    rr_renderer_draw_tile_hell_creek(this->renderer,
                                                     tile_index);
                else
                    rr_renderer_draw_tile_garden(this->renderer, tile_index);
                rr_renderer_context_state_free(this->renderer, &state);
            }
        }
#undef GRID_SIZE
        struct rr_simulation *sim = this->simulation;
        rr_simulation_create_component_vectors(sim);
        if (rr_frand() < 0.05)
        {
            EntityIdx petal_id = rr_simulation_alloc_entity(sim);
            struct rr_component_physical *physical =
                rr_simulation_add_physical(sim, petal_id);
            struct rr_component_petal *petal =
                rr_simulation_add_petal(sim, petal_id);
            struct rr_component_relations *relations =
                rr_simulation_add_relations(sim, petal_id);
            struct rr_component_health *health =
                rr_simulation_add_health(sim, petal_id);
            rr_component_physical_init(physical, sim);
            rr_component_petal_init(petal, sim);
            rr_component_relations_init(relations, sim);
            rr_component_health_init(health, sim);
            physical->radius = rr_frand() * 15 + 5;
            physical->lerp_x = -1050;
            physical->lerp_y = (rr_frand() - 0.5) * this->renderer->height;
            physical->y = physical->lerp_y;
            physical->on_title_screen = 1;
            uint32_t sum = 0;
            for (uint32_t i = 1; i < rr_petal_id_max; ++i)
                for (uint32_t r = 0; r < rr_rarity_id_max; ++r)
                    sum += this->inventory[i][r];
            float seed = rr_frand() * sum;
            uint8_t id_chosen = 1;
            uint8_t rarity_chosen = 0;
            for (uint32_t i = 1; i < rr_petal_id_max; ++i)
            {
                for (uint32_t r = 0; r < rr_rarity_id_max; ++r)
                    if ((seed -= this->inventory[i][r]) < 0)
                    {
                        id_chosen = i;
                        rarity_chosen = r;
                        break;
                    }
                if (seed < 0)
                    break;
            }
            petal->id = id_chosen;
            petal->rarity = rarity_chosen;
            if (id_chosen == rr_petal_id_uranium)
                physical->lerp_x -= 150;
            physical->velocity.x = rr_frand() * 40 + 80;
            physical->velocity.y = rr_frand() * 5 + 15;
            physical->animation_timer = rr_frand() * M_PI * 2;
            physical->parent_id = rand() % 3;
        }
        rr_system_particle_render_tick(this, &this->default_particle_manager,
                                       delta);
        struct rr_renderer_context_state state2;
        for (uint32_t i = 0; i < this->simulation->petal_count; ++i)
        {
            struct rr_component_physical *physical = rr_simulation_get_physical(
                sim, this->simulation->petal_vector[i]);
            physical->lerp_x += physical->velocity.x * delta;
            physical->lerp_y += physical->velocity.y * delta;
            physical->velocity.y +=
                (physical->y - physical->lerp_y) * delta * 1.25;
            physical->animation_timer += delta;
            physical->lerp_angle =
                physical->animation_timer * ((physical->parent_id % 3) - 1);
            rr_renderer_context_state_init(this->renderer, &state2);
            rr_renderer_translate(this->renderer, physical->lerp_x,
                                  physical->lerp_y);
            rr_component_petal_render(this->simulation->petal_vector[i], this,
                                      sim);
            rr_renderer_context_state_free(this->renderer, &state2);
            if (physical->lerp_x > 1200)
            {
                __rr_simulation_pending_deletion_free_components(
                    this->simulation->petal_vector[i], sim);
                __rr_simulation_pending_deletion_unset_entity(
                    this->simulation->petal_vector[i], sim);
            }
        }
        rr_system_particle_render_tick(this, &this->foreground_particle_manager,
                                       delta);
        rr_renderer_context_state_free(this->renderer, &state1);
    }
    // ui
    this->prev_focused = this->focused;
    this->cursor = rr_game_cursor_default;
    if (!this->block_ui_input)
    {
        this->window->poll_events(this->window, this);
        if (this->pressed != NULL && !rr_ui_mouse_over(this->pressed, this))
            this->pressed = NULL;
        if (this->focused != NULL)
            this->focused->on_event(this->focused, this);
        else
            this->window->on_event(this->window, this);
        if (this->prev_focused != this->focused && this->prev_focused != NULL)
            this->prev_focused->on_event(this->prev_focused, this);
    }
    this->block_ui_input = 0;
    this->block_fov_adjustment = 0;
    rr_ui_container_refactor(this->window, this);
    rr_ui_render_element(this->window, this);
    rr_dom_set_cursor(this->cursor);
    rr_game_crafting_tick(this, delta);
    rr_game_autocraft_tick(this, delta);
#ifndef EMSCRIPTEN
    lws_service(this->socket.socket_context, -1);
#endif
    if (this->socket_ready)
    {
        if (this->simulation_ready)
        {
            if (!this->is_mobile)
                write_serverbound_packet_desktop(this);
            else
                rr_write_serverbound_packet_mobile(this);
            rr_write_dev_cheat_packets(this, 0);
        }
        if ((!this->simulation_ready &&
             rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 13)) ||
            (this->simulation_ready && !this->cache.disable_leave_hotkey &&
             rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 27)))
        {
            if (!this->simulation_ready)
                rr_write_dev_cheat_packets(this, 1);
            struct proto_bug encoder;
            proto_bug_init(&encoder, RR_OUTGOING_PACKET);
            proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
            proto_bug_write_uint8(&encoder, rr_serverbound_squad_ready,
                                "header");
            rr_websocket_send(&this->socket, encoder.current - encoder.start);
        }
        if (!this->simulation_ready && !this->cache.disable_leave_hotkey &&
            rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 27))
        {
            this->socket_error = 0;
            struct proto_bug encoder;
            proto_bug_init(&encoder, RR_OUTGOING_PACKET);
            proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
            proto_bug_write_uint8(&encoder, rr_serverbound_squad_join, "header");
            proto_bug_write_uint8(&encoder, 3, "join type");
            rr_websocket_send(&this->socket, encoder.current - encoder.start);
        }
    }
    else if (!this->socket_pending)
        rr_game_connect_socket(this);
    if (!this->text_input_focused)
    {
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 186))
            this->cache.displaying_debug_information ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'M'))
            this->cache.use_mouse ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'H'))
            this->cache.show_hitboxes ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'I'))
            this->cache.hide_ui ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'K'))
            this->cache.hold_attack ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'L'))
            this->cache.hold_defense ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'P'))
            this->cache.low_performance_mode ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'O'))
            this->cache.show_loot ^= 1;
    }
    if (this->cache.hide_ui && this->simulation_ready)
        this->menu_open = 0;
    if (!this->block_fov_adjustment)
        this->player_info->fov_adjustment =
            rr_fclamp(this->player_info->fov_adjustment -
            this->input_data->scroll_delta * 0.001, 0, 1);

    if (this->cache.displaying_debug_information)
    {
        struct rr_renderer_context_state state;
        rr_renderer_context_state_init(this->renderer, &state);
        rr_renderer_set_text_size(this->renderer, 12);
        rr_renderer_set_line_width(this->renderer, 12 * 0.12);
        rr_renderer_set_text_baseline(this->renderer, 2);
        rr_renderer_set_fill(this->renderer, 0xffffffff);
        rr_renderer_set_text_align(this->renderer, 2);
        rr_renderer_translate(this->renderer, this->renderer->width,
                              this->renderer->height);
        rr_renderer_scale(this->renderer, this->renderer->scale);
        rr_renderer_translate(this->renderer, -5, -5);
        static char debug_mspt[100];
        long tick_sum = 0;
        long tick_max = 0;
        long frame_sum = 0;
        long frame_max = 0;
        for (uint32_t i = 0; i < RR_DEBUG_POLL_SIZE; ++i)
        {
            long t_t = this->debug_info.tick_times[i];
            long f_t = this->debug_info.frame_times[i];
            tick_sum += t_t;
            frame_sum += f_t;
            if (t_t > tick_max)
                tick_max = t_t;
            if (f_t > frame_max)
                frame_max = f_t;
        }

        sprintf(
            debug_mspt,
            "tick time (avg/max): %.1f/%.1f | frame time (avg/max): %.1f/%.1f",
            tick_sum * 0.001f / RR_DEBUG_POLL_SIZE, tick_max * 0.001f,
            frame_sum * 0.001f / RR_DEBUG_POLL_SIZE, frame_max * 0.001f);
        rr_renderer_stroke_text(this->renderer, debug_mspt, 0, 0);
        rr_renderer_fill_text(this->renderer, debug_mspt, 0, 0);
        sprintf(debug_mspt, "ctx calls: %d", rr_renderer_get_op_size());
        rr_renderer_context_state_free(this->renderer, &state);
        // rr_renderer_stroke_text
    }
    rr_renderer_context_state_free(this->renderer, &grand_state);

    rr_renderer_execute_instructions();
    rr_renderer_reset_instruction_queue();

    if (this->socket_ready)
        rr_websocket_send_all(&this->socket);
    gettimeofday(&end, NULL);
    long time_elapsed =
        (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    long frame_time = delta * 1000000.0f;

    this->debug_info.tick_times[this->debug_info.frame_pos] = time_elapsed;
    this->debug_info.frame_times[this->debug_info.frame_pos] = frame_time;
    this->debug_info.frame_pos =
        (this->debug_info.frame_pos + 1) % RR_DEBUG_POLL_SIZE;

    memset(this->input_data->keys_pressed_this_tick, 0, RR_BITSET_ROUND(256));
    memset(this->input_data->keys_released_this_tick, 0, RR_BITSET_ROUND(256));
    memset(this->input_data->keycodes_pressed_this_tick, 0,
           RR_BITSET_ROUND(256));
    this->input_data->mouse_buttons_up_this_tick = 0;
    this->input_data->mouse_buttons_down_this_tick = 0;
    this->input_data->mouse_state_this_tick = 0;
    this->input_data->keycodes_length = 0;
    this->input_data->clipboard = NULL;
    this->input_data->prev_mouse_x = this->input_data->mouse_x;
    this->input_data->prev_mouse_y = this->input_data->mouse_y;
}

void rr_game_connect_socket(struct rr_game *this)
{
    this->socket_ready = 0;
    this->simulation_ready = 0;
    this->socket_pending = 1;

#ifdef RIVET_BUILD
    rr_rivet_lobbies_find(this, NULL);
#else
    rr_websocket_init(&this->socket);
    this->socket.user_data = this;
    char url[128];
    rr_dom_get_socket_url(url);
    rr_websocket_connect_to(&this->socket, url);
    // rr_websocket_connect_to(&this->socket, "45.79.197.197", 1234, 0);
#endif
}

struct on_find_captures
{
    struct rr_game *game;
    struct rr_websocket *socket;
};

void rr_rivet_lobby_on_find(char *s, char *token, uint16_t port, void *_game)
{
    struct rr_game *game = _game;
    if (port == 0 || s == NULL || token == NULL)
    {
        // error;
        game->socket_error = 1;
        game->socket_pending = 0;
        game->socket_ready = 0;
        return;
    }
    // if (game->socket_ready)
    // rr_websocket_disconnect(&game->socket, game);
    rr_websocket_init(&game->socket);
    game->socket.user_data = game;
    game->socket_pending = 1;
    char link[256];
    sprintf(link, "ws%s://%s:%u\n", port == 443 ? "s" : "", s, port);
    memcpy(game->rivet_player_token, token, 400);
    rr_websocket_connect_to(&game->socket, link);
}
