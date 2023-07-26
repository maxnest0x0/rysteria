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
#include <Client/InputData.h>
#include <Client/Renderer/ComponentRender.h>
#include <Client/Renderer/RenderFunctions.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Socket.h>
#include <Client/Storage.h>
#include <Client/Ui/Engine.h>
#include <Shared/Api.h>
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

void validate_loadout(struct rr_game *this)
{
    uint32_t temp_inv[rr_petal_id_max][rr_rarity_id_max];
    memcpy(&temp_inv, &this->inventory,
           (sizeof(uint32_t)) * rr_petal_id_max * rr_rarity_id_max);
    for (uint8_t i = 0; i < 20; ++i)
    {
        uint8_t id = this->settings.loadout[i].id;
        uint8_t rarity = this->settings.loadout[i].rarity;
        if (temp_inv[id][rarity] == 0)
            this->settings.loadout[i].id = this->settings.loadout[i].rarity = 0;
        else
            --temp_inv[id][rarity];
    }
}

void rr_api_on_get_petals(char *json, void *a)
{
    struct rr_game *game = a;

    for (uint32_t id = 0; id < rr_petal_id_max; ++id)
        for (uint32_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
            game->inventory[id][rarity] = 0;
    
    cJSON *parsed = cJSON_Parse(json);
    if (parsed == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return;
    }

    cJSON *petals = cJSON_GetObjectItemCaseSensitive(parsed, "petals");
    if (petals == NULL || !cJSON_IsObject(petals))
    {
        fprintf(stderr, "petals is missing or is not an object\n");
        cJSON_Delete(parsed);
        return;
    }
    for (cJSON *petal_key = petals->child; petal_key != NULL;
         petal_key = petal_key->next)
    {
        char *key = petal_key->string;
        char *sub_key1 = strtok(key, ":");
        char *sub_key2 = strtok(NULL, ":");

        if (sub_key1 && sub_key2)
        {
            int index1 = atoi(sub_key1);
            int index2 = atoi(sub_key2);

            game->inventory[index1][index2] =
                petal_key->valueint; // Assuming the value is a double.
        }
    }

    cJSON_Delete(parsed);
}

void rr_rivet_on_log_in(char *token, char *avatar_url, char *name,
                        char *account_number, char *uuid, void *captures)
{
    struct rr_game *this = captures;
    strcpy(this->rivet_account.token, token);
    strcpy(this->rivet_account.name, name);
    strcpy(this->rivet_account.avatar_url, avatar_url);
    strcpy(this->rivet_account.account_number, account_number);
    strcpy(this->rivet_account.uuid, uuid);

    rr_api_get_petals(this->rivet_account.uuid, this->rivet_account.token,
                      this);
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

static uint8_t socket_ready(struct rr_ui_element *this, struct rr_game *game)
{
    return game->socket_ready;
}

static uint8_t socket_pending_or_ready(struct rr_ui_element *this, struct rr_game *game)
{
    return game->socket_pending || game->socket_ready;
}

static void window_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
        game->bottom_ui_open = game->top_ui_open = 0;
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
    this->settings.slots_unlocked = 10;
    for (uint8_t i = 0; i < this->settings.slots_unlocked; ++i)
        this->protocol_state |= ((1 | (1 << 10)) << i);

    this->inventory[rr_petal_id_basic][rr_rarity_id_common] = 1;

    strcpy(this->rivet_account.name, "loading");
    strcpy(this->rivet_account.avatar_url, "");
    strcpy(this->rivet_account.token, "");
    strcpy(this->rivet_account.account_number, "#0000");
    strcpy(this->rivet_account.uuid, "no-uuid");
    rr_rivet_identities_create_guest(this);

    rr_ui_container_add_element(
        this->window,
        rr_ui_pad(rr_ui_set_justify(rr_ui_rivet_container_init(this), -1, -1),
                  10));
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(rr_ui_wave_container_init(), simulation_ready));

    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_background(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 10, 20, 3,
                    rr_ui_v_container_init(rr_ui_container_init(), 0, 20, 4,
                        rr_ui_text_init("rrolf.io", 96, 0xffffffff),
                        rr_ui_h_container_init(
                            rr_ui_container_init(), 10, 20, 2,
                            rr_ui_text_init("name input (TODO)", 25, 0xffffffff),
                            rr_ui_set_background(rr_ui_join_button_init(),
                                             0xff1dd129)
                        ),
                        rr_ui_set_justify(rr_ui_set_background(rr_ui_squad_button_init(), 0xff1dd129), 1, 0),
                    rr_ui_set_background(
                        rr_ui_link_toggle(
                            rr_ui_v_container_init(
                                rr_ui_container_init(), 10, 20, 2,
                                rr_ui_h_container_init(
                                    rr_ui_container_init(), 1, 15, 2,
                                    rr_ui_text_init("Squad", 18, 0xffffffff),
                                    rr_ui_info_init(), -1, 0),
                                rr_ui_choose_element_init(
                                    rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                                    rr_ui_h_container_init(
                                        rr_ui_container_init(), 10, 20, 4,
                                        rr_ui_squad_player_container_init(
                                            &this->squad_members[0]),
                                        rr_ui_squad_player_container_init(
                                            &this->squad_members[1]),
                                        rr_ui_squad_player_container_init(
                                            &this->squad_members[2]),
                                        rr_ui_squad_player_container_init(
                                            &this->squad_members[3])),
                                    rr_ui_set_justify(rr_ui_countdown_init(this), 1,
                                                    0)
                                    ),
                                rr_ui_text_init("Joining Squad...", 24, 0xffffffff),
                                socket_ready)),
                            socket_pending_or_ready),
                        0x40ffffff)),
                                        rr_ui_h_container_init(rr_ui_container_init(), 0, 15, 10,
                                           rr_ui_title_screen_loadout_button_init(0),
                                           rr_ui_title_screen_loadout_button_init(1),
                                           rr_ui_title_screen_loadout_button_init(2),
                                           rr_ui_title_screen_loadout_button_init(3),
                                           rr_ui_title_screen_loadout_button_init(4),
                                           rr_ui_title_screen_loadout_button_init(5),
                                           rr_ui_title_screen_loadout_button_init(6),
                                           rr_ui_title_screen_loadout_button_init(7),
                                           rr_ui_title_screen_loadout_button_init(8),
                                           rr_ui_title_screen_loadout_button_init(9)),
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 15, 10,
                                           rr_ui_title_screen_loadout_button_init(10),
                                           rr_ui_title_screen_loadout_button_init(11),
                                           rr_ui_title_screen_loadout_button_init(12),
                                           rr_ui_title_screen_loadout_button_init(13),
                                           rr_ui_title_screen_loadout_button_init(14),
                                           rr_ui_title_screen_loadout_button_init(15),
                                           rr_ui_title_screen_loadout_button_init(16),
                                           rr_ui_title_screen_loadout_button_init(17),
                                           rr_ui_title_screen_loadout_button_init(18),
                                           rr_ui_title_screen_loadout_button_init(19)),
                    rr_ui_text_init("powered by Rivet", 15, 0xffffffff)),
                0x00000000),
            simulation_not_ready));
    rr_ui_container_add_element(this->window, rr_ui_inventory_container_init());
    rr_ui_container_add_element(this->window, rr_ui_mob_container_init());
    rr_ui_container_add_element(this->window, rr_ui_crafting_container_init());
    rr_ui_container_add_element(this->window, rr_ui_waiting_respawn_screen_init());
    rr_ui_container_add_element(this->window, rr_ui_finished_game_screen());
    rr_ui_container_add_element(this->window, rr_ui_pad(
        rr_ui_v_pad(
            rr_ui_set_justify(
                rr_ui_v_container_init(rr_ui_container_init(), 10, 20, 4,
                    rr_ui_in_game_player_hud_init(0),
                    rr_ui_in_game_player_hud_init(1),
                    rr_ui_in_game_player_hud_init(2),
                    rr_ui_in_game_player_hud_init(3)
                )
            , -1, -1)
        , 100)
    , 50));
    rr_ui_container_add_element(
        this->window,
        rr_ui_set_justify(
            rr_ui_link_toggle(
                rr_ui_v_container_init(rr_ui_container_init(), 10, 10, 3,
                                       rr_ui_inventory_toggle_button_init(),
                                       rr_ui_mob_gallery_toggle_button_init(),
                                       rr_ui_crafting_toggle_button_init()),
                simulation_not_ready),
            -1, 1));
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 15, 15, 2,
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15, 12,
                        rr_ui_text_init("[x]", 18, 0xffffffff),
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
                        rr_ui_text_init("[x]", 18, 0x00000000)),
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15, 10,
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(10),
                            rr_ui_text_init("[1]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(11),
                            rr_ui_text_init("[2]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(12),
                            rr_ui_text_init("[3]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(13),
                            rr_ui_text_init("[4]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(14),
                            rr_ui_text_init("[5]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(15),
                            rr_ui_text_init("[6]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(16),
                            rr_ui_text_init("[7]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(17),
                            rr_ui_text_init("[8]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(18),
                            rr_ui_text_init("[9]", 14, 0xffffffff)
                        ),
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 10, 2, 
                            rr_ui_loadout_button_init(19),
                            rr_ui_text_init("[0]", 14, 0xffffffff)
                        )
                    )),
                0, 1),
            simulation_ready));
    rr_ui_container_add_element(this->window,
                                rr_ui_settings_container_init(this));
    this->squad_info_tooltip = rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_set_background(
                    rr_ui_v_container_init(
                        rr_ui_container_init(), 10, 10, 2,
                        rr_ui_set_justify(
                            rr_ui_h_container_init(
                                rr_ui_container_init(), 5, 10, 2,
                                rr_ui_flower_init(0, 35),
                                rr_ui_text_init("- ready", 15, 0xffffffff)),
                            -1, 0),
                        rr_ui_set_justify(
                            rr_ui_h_container_init(
                                rr_ui_container_init(), 5, 10, 2,
                                rr_ui_flower_init(1, 35),
                                rr_ui_text_init("- not ready", 15, 0xffffffff)),
                            -1, 0)),
                    0x80000000),
                -1, -1),
            rr_ui_never_show));
    this->squad_info_tooltip->poll_events = rr_ui_no_focus;

    this->rivet_info_tooltip = rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(rr_ui_set_background(
                                  rr_ui_h_container_init(
                                      rr_ui_container_init(), 10, 0, 1,
                                      rr_ui_text_init(this->rivet_account.uuid,
                                                      16, 0xffffffff)),
                                  0x80000000),
                              -1, -1),
            rr_ui_never_show));
    this->rivet_info_tooltip->poll_events = rr_ui_no_focus;
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
            this->inventory[id][rarity] = 0;
            rr_renderer_init(&this->static_petals[id][rarity]);
            rr_renderer_set_dimensions(&this->static_petals[id][rarity], 50,
                                       50);
            rr_renderer_translate(&this->static_petals[id][rarity], 25, 25);
            rr_renderer_render_static_petal(&this->static_petals[id][rarity],
                                            id, rarity);
            this->petal_tooltips[id][rarity] =
                rr_ui_petal_tooltip_init(id, rarity);
            rr_ui_container_add_element(this->window,
                                        this->petal_tooltips[id][rarity]);
            // remember that these don't have a container
        }
    }

    rr_assets_init(this);

    rr_local_storage_get_bytes("debug",
                               &this->settings.displaying_debug_information);
    rr_local_storage_get_bytes("loadout", &this->settings.loadout);
    rr_local_storage_get_bytes("props", &this->settings.map_props);
    rr_local_storage_get_bytes("screen_shake", &this->settings.screen_shake);
    rr_local_storage_get_bytes("ui_hitboxes", &this->settings.show_ui_hitbox);
    rr_local_storage_get_bytes("slots_count", &this->settings.slots_unlocked);
    rr_local_storage_get_bytes("mouse", &this->settings.use_mouse);
    // rr_local_storage_get_bytes("settings", &this->settings);
    // while (at < size)
    // {
    //     this->settings.loadout[storage_result[at]].id = storage_result[at +
    //     1]; this->settings.loadout[storage_result[at]].rarity =
    //     storage_result[at + 2]; at += 3;
    // }
}

void rr_game_websocket_on_event_function(enum rr_websocket_event_type type,
                                         void *data, void *captures,
                                         uint64_t size)
{
    struct rr_game *this = captures;
    switch (type)
    {
    case rr_websocket_event_type_open:
        this->socket_pending = 0;
        puts("websocket opened");
        break;
    case rr_websocket_event_type_close:
        // memset the clients
        puts("websocket closed");
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
            struct proto_bug verify_encoder;
            proto_bug_init(&verify_encoder, &output_packet[0]);
            proto_bug_write_uint64(&verify_encoder, rr_get_rand(),
                                   "useless bytes");
            proto_bug_write_uint64(&verify_encoder, verification,
                                   "verification");
            uint64_t token_size = strlen(this->socket.rivet_player_token);
            uint64_t uuid_size = strlen(this->socket.uuid);
            proto_bug_write_varuint(&verify_encoder, token_size,
                                    "rivet token size");
            proto_bug_write_varuint(&verify_encoder, uuid_size, "uuid size");
            proto_bug_write_string(&verify_encoder,
                                   this->socket.rivet_player_token, token_size,
                                   "rivet token");
            proto_bug_write_string(&verify_encoder, this->socket.uuid,
                                   uuid_size, "rivet uuid");
            rr_websocket_send(&this->socket, verify_encoder.start,
                              verify_encoder.current);
            this->socket_ready = 1;
            return;
        }

        this->socket.clientbound_encryption_key =
            rr_get_hash(this->socket.clientbound_encryption_key);
        rr_decrypt(data, size, this->socket.clientbound_encryption_key);
        switch (proto_bug_read_uint8(&encoder, "header"))
        {
        case 0:
        {
            this->simulation_ready = 1;
            rr_simulation_read_binary(this, &encoder);
            struct proto_bug encoder2;
            proto_bug_init(&encoder2, output_packet);
            proto_bug_write_uint8(&encoder2, 0, "header");
            proto_bug_write_uint8(&encoder2, 0, "movement type");
            uint8_t movement_flags = 0;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 87) ||
                 rr_bitset_get(this->input_data->keys_pressed, 38))
                << 0;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 65) ||
                 rr_bitset_get(this->input_data->keys_pressed, 37))
                << 1;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 83) ||
                 rr_bitset_get(this->input_data->keys_pressed, 40))
                << 2;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 68) ||
                 rr_bitset_get(this->input_data->keys_pressed, 39))
                << 3;
            movement_flags |= this->input_data->mouse_buttons << 4;
            movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 32)
                              << 4;
            movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 16)
                              << 5;
            movement_flags |= this->settings.use_mouse << 6;
            proto_bug_write_uint8(&encoder2, movement_flags,
                                  "movement kb flags");
            proto_bug_write_float32(&encoder2,
                                    this->input_data->mouse_x -
                                        this->renderer->width / 2,
                                    "mouse x");
            proto_bug_write_float32(&encoder2,
                                    this->input_data->mouse_y -
                                        this->renderer->height / 2,
                                    "mouse y");
            rr_websocket_send(&this->socket, encoder2.start, encoder2.current);
            break;
        }
        case 69:
        {
            this->ticks_until_game_start =
                proto_bug_read_uint8(&encoder, "countdown");
            for (uint32_t i = 0; i < 4; ++i)
            {
                this->squad_members[i].in_use =
                    proto_bug_read_uint8(&encoder, "bitbit");
                this->squad_members[i].ready =
                    proto_bug_read_uint8(&encoder, "ready");
                for (uint32_t j = 0; j < 20; ++j)
                {
                    this->squad_members[i].loadout[j].id =
                        proto_bug_read_uint8(&encoder, "id");
                    this->squad_members[i].loadout[j].rarity =
                        proto_bug_read_uint8(&encoder, "rar");
                }
            }
            struct proto_bug encoder2;
            proto_bug_init(&encoder2, output_packet);
            proto_bug_write_uint8(&encoder2, 70, "header");
            for (uint32_t i = 0; i < this->settings.slots_unlocked; ++i)
            {
                if (this->protocol_state & (1 << i))
                {
                    proto_bug_write_uint8(&encoder2, i + 1, "pos");
                    proto_bug_write_uint8(&encoder2,
                                          this->settings.loadout[i].id, "id");
                    proto_bug_write_uint8(
                        &encoder2, this->settings.loadout[i].rarity, "rar");
                }
                if (this->protocol_state & (1 << (i + 10)))
                {
                    proto_bug_write_uint8(&encoder2, i + 1 + 10, "pos");
                    proto_bug_write_uint8(&encoder2,
                                          this->settings.loadout[i + 10].id, "id");
                    proto_bug_write_uint8(
                        &encoder2, this->settings.loadout[i + 10].rarity, "rar");
                }
            }
            proto_bug_write_uint8(&encoder2, 0, "pos");
            rr_websocket_send(&this->socket, encoder2.start, encoder2.current);
            this->protocol_state = 0;
            break;
        }
        default:
            RR_UNREACHABLE("how'd this happen");
        }
        break;
    }
    default:
        RR_UNREACHABLE("non exhaustive switch expression");
    }
}

void render_drop_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_drop_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_health_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x,
                          physical->lerp_y + physical->radius + 30);
    rr_component_health_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_mob_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_mob_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_petal_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_petal_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_flower_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_flower_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_web_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_web_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void player_info_finder(struct rr_game *this)
{
    struct rr_simulation *simulation = this->simulation;
    uint8_t counter = 1;
    memset(&this->player_infos, 0, sizeof this->player_infos);
    this->player_infos[0] = this->player_info->parent_id;
    for (EntityIdx i = 1; i < RR_MAX_ENTITY_COUNT; ++i)
        if (rr_bitset_get(simulation->player_info_tracker, i) &&
            i != this->player_info->parent_id)
            this->player_infos[counter++] = i;
}

static void render_background(struct rr_component_player_info *player_info,
                              struct rr_game *this, uint32_t prop_amount)
{
    if (this->settings.ourpetsnake_mode)
        return;
    double scale = player_info->lerp_camera_fov * this->renderer->scale;
    double leftX =
        player_info->lerp_camera_x - this->renderer->width / (2 * scale);
    double rightX =
        player_info->lerp_camera_x + this->renderer->width / (2 * scale);
    double topY =
        player_info->lerp_camera_y - this->renderer->height / (2 * scale);
    double bottomY =
        player_info->lerp_camera_y + this->renderer->height / (2 * scale);

#define GRID_SIZE (256)
    double newLeftX = floorf(leftX / GRID_SIZE) * GRID_SIZE;
    double newTopY = floorf(topY / GRID_SIZE) * GRID_SIZE;
    for (; newLeftX < rightX; newLeftX += GRID_SIZE)
    {
        for (double currY = newTopY; currY < bottomY; currY += GRID_SIZE)
        {
            uint32_t tile_index = (uint32_t)((newLeftX / GRID_SIZE + 1) *
                                             (currY / GRID_SIZE + 2)) %
                                  this->tiles_size;
            struct rr_renderer_context_state state;
            rr_renderer_context_state_init(this->renderer, &state);
            rr_renderer_translate(this->renderer, newLeftX + GRID_SIZE / 2,
                                  currY + GRID_SIZE / 2);
            rr_renderer_draw_image(this->renderer, this->tiles + tile_index);
            rr_renderer_context_state_free(this->renderer, &state);
        }
    }

#define render_map_feature                                                     \
    struct rr_renderer_context_state state;                                    \
    float theta = ((double)(uint32_t)(rr_get_hash(i + 200000))) /              \
                  ((double)UINT32_MAX) * (M_PI * 2);                           \
    float distance = sqrtf(((double)(uint32_t)(rr_get_hash(i + 300000))) /     \
                           ((double)UINT32_MAX)) *                             \
                     1600.0;                                                   \
    float rotation = ((double)(uint32_t)(rr_get_hash(i + 400000))) /           \
                     ((double)UINT32_MAX) * (M_PI * 2);                        \
    float x = distance * sinf(theta);                                          \
    float y = distance * cosf(theta);                                          \
    if (x < leftX - (selected_feature == 8 ? 400 : 100))                       \
        continue;                                                              \
    if (x > rightX + (selected_feature == 8 ? 400 : 100))                      \
        continue;                                                              \
    if (y < topY - (selected_feature == 8 ? 400 : 100))                        \
        continue;                                                              \
    if (y > bottomY + (selected_feature == 8 ? 400 : 100))                     \
        continue;                                                              \
    rr_renderer_context_state_init(this->renderer, &state);                    \
    rr_renderer_translate(this->renderer, x, y);                               \
    rr_renderer_rotate(this->renderer, rotation);                              \
    rr_renderer_draw_image(this->renderer,                                     \
                           &this->background_features[selected_feature]);      \
    rr_renderer_context_state_free(this->renderer, &state);

    rr_renderer_set_global_alpha(this->renderer, 0.75f);

    // draw background features
    for (uint64_t i = 0; i < prop_amount;)
    {
        uint64_t selected_feature;
        // any number between 0-8 and is not 1. 1 is water lettuce
        // which is disabled for now
        do
        {
            selected_feature = rr_get_hash(i) % 8;
            i++;
        } while (selected_feature == 1);

        render_map_feature
    }
    // trees over everything
    for (uint64_t i = 0; i < prop_amount / 50; i++)
    {
        uint64_t selected_feature = 8;
        render_map_feature
    }

#undef GRID_SIZE
#undef render_map_feature
}

void rr_game_tick(struct rr_game *this, float delta)
{
    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    if (this->simulation_ready)
        validate_loadout(this);
    // rr_storage_layout_save(this);
    rr_local_storage_store_bytes(
        "debug", &this->settings.displaying_debug_information,
        sizeof this->settings.displaying_debug_information);
    rr_local_storage_store_bytes("loadout", &this->settings.loadout,
                                 sizeof this->settings.loadout);
    rr_local_storage_store_bytes("props", &this->settings.map_props,
                                 sizeof this->settings.map_props);
    rr_local_storage_store_bytes("screen_shake", &this->settings.screen_shake,
                                 sizeof this->settings.screen_shake);
    rr_local_storage_store_bytes("ui_hitboxes", &this->settings.show_ui_hitbox,
                                 sizeof this->settings.show_ui_hitbox);
    rr_local_storage_store_bytes("slots_count", &this->settings.slots_unlocked,
                                 sizeof this->settings.slots_unlocked);
    rr_local_storage_store_bytes("mouse", &this->settings.use_mouse,
                                 sizeof this->settings.use_mouse);

    // rr_local_storage_store_bytes("settings", &this->settings,
    //                              sizeof this->settings);
    double time = start.tv_sec * 1000000 + start.tv_usec;
    rr_renderer_set_transform(this->renderer, 1, 0, 0, 0, 1, 0);
    // rr_renderer_set_grayscale(this->renderer, this->grayscale * 100);
    struct rr_renderer_context_state grand_state;
    rr_renderer_context_state_init(this->renderer, &grand_state);
    // render off-game elements

    if (this->simulation_ready)
    {
        rr_simulation_tick(this->simulation, delta);

        this->renderer->state.filter.amount = 0;
        struct rr_renderer_context_state state1;
        struct rr_renderer_context_state state2;
        player_info_finder(this);
        if (this->player_info != NULL)
        {
            // screen shake
            rr_renderer_context_state_init(this->renderer, &state1);
            struct rr_component_player_info *player_info = this->player_info;
            rr_renderer_translate(this->renderer, this->renderer->width / 2,
                                  this->renderer->height / 2);
            rr_renderer_scale(this->renderer, player_info->lerp_camera_fov *
                                                  this->renderer->scale);
            rr_renderer_translate(this->renderer, -player_info->lerp_camera_x,
                                  -player_info->lerp_camera_y);

            if (this->settings.screen_shake &&
                player_info->flower_id != RR_NULL_ENTITY)
            {
                if (rr_simulation_get_physical(this->simulation,
                                               player_info->flower_id)
                        ->server_animation_tick != 0)
                {
                    float r = rr_frand() * 5;
                    float a = rr_frand() * 2 * M_PI;
                    rr_renderer_translate(this->renderer, r * cosf(a),
                                          r * sinf(a));
                }
            }

            uint32_t alpha = (uint32_t)(player_info->lerp_camera_fov * 51)
                             << 24;
            rr_renderer_context_state_init(this->renderer, &state2);
            rr_renderer_set_transform(this->renderer, 1, 0, 0, 0, 1, 0);
            rr_renderer_set_fill(this->renderer, 0xff45230a);
            rr_renderer_fill_rect(this->renderer, 0, 0, this->renderer->width,
                                  this->renderer->height);
            rr_renderer_set_fill(this->renderer, alpha);
            rr_renderer_fill_rect(this->renderer, 0, 0, this->renderer->width,
                                  this->renderer->height);
            rr_renderer_context_state_free(this->renderer, &state2);
            rr_renderer_context_state_init(this->renderer, &state2);

            struct rr_component_arena *arena =
                rr_simulation_get_arena(this->simulation, 1);
            rr_renderer_begin_path(this->renderer);
            rr_renderer_arc(this->renderer, 0, 0, arena->radius);
            rr_renderer_set_fill(this->renderer, 0xff45230a);
            rr_renderer_fill(this->renderer);
            rr_renderer_clip(this->renderer);

            rr_renderer_set_line_width(this->renderer, 1.0f);
            rr_renderer_set_stroke(this->renderer, alpha);
            rr_renderer_set_global_alpha(this->renderer, 1);

            render_background(player_info, this,
                              this->settings.map_props * 750);

            rr_renderer_context_state_free(this->renderer, &state2);

            rr_simulation_for_each_health(this->simulation, this,
                                          render_health_component);
            rr_simulation_for_each_drop(this->simulation, this,
                                        render_drop_component);
            rr_simulation_for_each_web(this->simulation, this,
                                          render_web_component);
            rr_simulation_for_each_mob(this->simulation, this,
                                       render_mob_component);
            rr_simulation_for_each_petal(this->simulation, this,
                                         render_petal_component);
            rr_simulation_for_each_flower(this->simulation, this,
                                          render_flower_component);
            rr_renderer_context_state_free(this->renderer, &state1);
        }
    }
    else
    {
        // render background but different
        struct rr_component_player_info custom_player_info;
        rr_component_player_info_init(&custom_player_info, 0);
        custom_player_info.lerp_camera_fov = 0.9;
        struct rr_renderer_context_state state;
        rr_renderer_context_state_init(this->renderer, &state);
        rr_renderer_translate(this->renderer, this->renderer->width * 0.5f,
                              this->renderer->height * 0.5f);
        render_background(&custom_player_info, this,
                          this->settings.map_props * 750);
        rr_renderer_context_state_free(this->renderer, &state);
    }
    // ui
    this->crafting_data.animation -= delta;
    if (this->crafting_data.animation < 0)
        this->crafting_data.animation = 0;
    this->prev_focused = this->focused;
    rr_ui_container_refactor(this->window);
    rr_ui_render_element(this->window, this);
    if (!this->block_ui_input)
    {
        this->window->poll_events(this->window, this);

        if (this->focused != NULL)
            this->focused->on_event(this->focused, this);
        else
            this->window->on_event(this->window, this);
        if (this->prev_focused != this->focused && this->prev_focused != NULL)
            this->prev_focused->on_event(this->prev_focused, this);
    }
    this->block_ui_input = 0;
#ifndef EMSCRIPTEN
    lws_service(this->socket.socket_context, -1);
#endif
    if (this->socket_ready)
    {
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              75 /* k */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 2, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              76 /* l */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 1, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              86 /* v */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 3, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (this->simulation_ready)
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 2, "header");
            uint8_t should_write = 0;
            uint8_t switch_all = rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 'X');
            for (uint8_t n = 0; n < this->settings.slots_unlocked; ++n)
                if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick, 48 + n) || switch_all)
                {
                    proto_bug_write_uint8(&encoder, n == 0 ? 10 : n, "petal switch");
                    should_write = 1;
                }
            if (should_write)
            {
                proto_bug_write_uint8(&encoder, 0, "petal switch");
                rr_websocket_send(&this->socket, encoder.start, encoder.current);
            }
        }
    }
    if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                          186 /* ; */))
        this->settings.displaying_debug_information ^= 1;
    gettimeofday(&end, NULL);
    long time_elapsed =
        (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    this->debug_info.cumulative_tick_time += time_elapsed;
    if (time_elapsed > this->debug_info.max_tick_time)
        this->debug_info.max_tick_time = time_elapsed;
    long frame_time =
        end.tv_sec * 1000000 + end.tv_usec - this->debug_info.last_tick_time;
    this->debug_info.cumulative_frame_time += frame_time;
    if (frame_time > this->debug_info.max_frame_time)
        this->debug_info.max_frame_time = frame_time;

    if (this->settings.displaying_debug_information)
    {
        struct rr_renderer_context_state state;
        rr_renderer_context_state_init(this->renderer, &state);
        rr_renderer_set_text_size(this->renderer, 12);
        rr_renderer_set_line_width(this->renderer, 12 * 0.12);
        rr_renderer_set_text_baseline(this->renderer, 2);
        rr_renderer_set_fill(this->renderer, 0xffffffff);
        rr_renderer_set_text_align(this->renderer, 2);
        rr_renderer_translate(this->renderer, this->renderer->width - 5,
                              this->renderer->height - 5);
        static char debug_mspt[100];
        debug_mspt[sprintf(
            debug_mspt,
            "tick time (avg/max): %.1f/%.1f | frame time (avg/max): %.1f/%.1f",
            this->debug_info.cumulative_tick_time * 0.001f /
                (this->debug_info.count + 1),
            this->debug_info.max_tick_time * 0.001f,
            this->debug_info.cumulative_frame_time * 0.001f /
                (this->debug_info.count + 1),
            this->debug_info.max_frame_time * 0.001f)] = 0;
        rr_renderer_stroke_text(this->renderer, debug_mspt, 0, 0);
        rr_renderer_fill_text(this->renderer, debug_mspt, 0, 0);
        rr_renderer_context_state_free(this->renderer, &state);
        // rr_renderer_stroke_text
    }
    rr_renderer_context_state_free(this->renderer, &grand_state);

    rr_renderer_execute_order_66();
    memset(this->input_data->keys_pressed_this_tick, 0, RR_BITSET_ROUND(256));
    memset(this->input_data->keys_released_this_tick, 0, RR_BITSET_ROUND(256));
    this->input_data->mouse_buttons_up_this_tick = 0;
    this->input_data->mouse_buttons_down_this_tick = 0;
    this->input_data->mouse_state_this_tick = 0;
    this->input_data->prev_mouse_x = this->input_data->mouse_x;
    this->input_data->prev_mouse_y = this->input_data->mouse_y;
    if (++this->debug_info.count == 10)
        memset(&this->debug_info, 0, sizeof this->debug_info);
    this->debug_info.last_tick_time = end.tv_sec * 1000000 + end.tv_usec;
}

void rr_game_connect_socket(struct rr_game *this)
{
    memset(this->simulation, 0, sizeof *this->simulation);
    for (uint8_t i = 0; i < this->settings.slots_unlocked; ++i)
        this->protocol_state |= ((1 | (1 << 10)) << i);
    this->socket_ready = 0;
    this->simulation_ready = 0;
    this->socket_pending = 1;
    rr_websocket_init(&this->socket);
    this->socket.user_data = this;
    this->socket.on_event = rr_game_websocket_on_event_function;

#ifdef RIVET_BUILD
    rr_rivet_lobbies_find(this);
#else
#ifdef RR_WINDOWS
    rr_websocket_connect_to(&this->socket, "127.0.0.1", 1234, 0);
#else
    // for testing
    // if (!this->socket.rivet_player_token)
    // {
    //     this->socket.rivet_player_token = calloc(10, 1);
    //     this->socket.uuid = calloc(10, 1);
    // }
    rr_websocket_connect_to(&this->socket, "127.0.0.1", 1234, 0);
    // rr_websocket_connect_to(&this->socket, "45.79.197.197", 1234, 0);
#endif
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
    // rr_websocket_connect_to(&game->socket, "127.0.0.1", 1234, 0);

    if (port == 443)
        rr_websocket_connect_to(&game->socket, s, port, 1);
    else
        rr_websocket_connect_to(&game->socket, s, port, 0);
    free(s);
    // captures->socket->rivet_player_token = strdup(token);
    // free(token);
    game->socket.rivet_player_token = token;
    game->socket.uuid = game->rivet_account.uuid;
}
