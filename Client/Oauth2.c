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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Client/DOM.h>
#include <Client/Game.h>
#include <Client/Platform.h>
#include <Client/Socket.h>
#include <Client/Storage.h>
#include <Shared/StaticData.h>
#include <Shared/pb.h>

// Everything about accounts (linking Discord / CrazyGames, transfer codes)
// goes to the game server over the game's own websocket, which hands it to the
// master server. The browser never talks to the master server.
//
// A request waits in game->auth_* until the connection is verified, then is
// sent once. The answer (rr_auth_read_reply) shows its message; if it moved the
// player to another account, the game waits a moment so the message can be
// read, then disconnects once and reconnects with that account, which is the
// one clean way to load it.

// seconds the "linked" message stays up before reconnecting
#define RECONNECT_DELAY 1.5f
// seconds a request waits for a connection before giving up
#define REQUEST_TIMEOUT 12.0f

static void set_message(struct rr_game *this, char const *message)
{
    snprintf(this->auth_message, sizeof this->auth_message, "%s", message);
}

// Queues a request. kind is a rr_account_request_kind, payload the Discord
// code, CrazyGames token or transfer code ("" for a new transfer code).
void rr_auth_request(void *game, uint8_t kind, char const *payload)
{
    struct rr_game *this = game;
    if (this->auth_reconnect_in > 0)
        return;
    snprintf(this->auth_payload, sizeof this->auth_payload, "%s", payload);
    this->auth_kind = kind;
    this->auth_pending = 1;
    this->auth_wait = 0;
    set_message(this, kind == rr_account_request_code_create
                          ? "Making a code..."
                          : "Checking...");
}

void rr_auth_get_transfer_code(void *game)
{
    struct rr_game *this = game;
    this->transfer_code[0] = 0;
    rr_auth_request(game, rr_account_request_code_create, "");
}

void rr_auth_use_transfer_code(void *game)
{
    struct rr_game *this = game;
    if (this->enter_code[0] == 0)
    {
        set_message(this, "Type the code first.");
        return;
    }
    rr_auth_request(game, rr_account_request_code_redeem, this->enter_code);
}

// Runs once per frame: sends the waiting request when the connection is
// ready, and does the reconnect after an account change.
void rr_auth_tick(struct rr_game *this, float delta)
{
    if (this->auth_reconnect_in > 0)
    {
        this->auth_reconnect_in -= delta;
        if (this->auth_reconnect_in <= 0)
        {
            this->auth_reconnect_in = 0;
            this->auth_reconnected = 1;
            this->account_ready = 0;
            rr_game_connect_socket(this);
        }
        return;
    }
    if (this->auth_reconnected && this->account_ready)
    {
        this->auth_reconnected = 0;
        set_message(this, "Done, the linked account is loaded.");
    }
    if (!this->auth_pending)
        return;
    if (!this->socket_ready || !this->account_ready)
    {
        this->auth_wait += delta;
        if (this->auth_wait > REQUEST_TIMEOUT)
        {
            this->auth_pending = 0;
            set_message(this, "Could not reach the game server.");
        }
        return;
    }
    this->auth_pending = 0;
    struct proto_bug encoder;
    proto_bug_init(&encoder, RR_OUTGOING_PACKET);
    proto_bug_write_uint8(&encoder, this->socket.quick_verification, "qv");
    proto_bug_write_uint8(&encoder, rr_serverbound_account_request, "header");
    proto_bug_write_uint8(&encoder, this->auth_kind, "kind");
    // transfer codes only work between CrazyGames and the web, see the master
    proto_bug_write_uint8(&encoder, rr_platform_on_crazygames(), "crazygames");
    proto_bug_write_string(&encoder, this->auth_payload,
                           sizeof this->auth_payload, "payload");
    rr_websocket_send(&this->socket, encoder.current - encoder.start);
    memset(this->auth_payload, 0, sizeof this->auth_payload);
}

// The game server's answer to a request, see Server.c
void rr_auth_read_reply(void *game, void *_decoder)
{
    struct rr_game *this = game;
    struct proto_bug *decoder = _decoder;
    uint8_t ok = proto_bug_read_uint8(decoder, "ok");
    uint8_t kind = proto_bug_read_uint8(decoder, "kind");
    char message[96] = {0};
    char uuid[sizeof this->rivet_account.uuid] = {0};
    char token[sizeof this->rivet_account.token] = {0};
    char code[sizeof this->transfer_code] = {0};
    proto_bug_read_string(decoder, message, sizeof message - 1, "message");
    proto_bug_read_string(decoder, uuid, sizeof uuid - 1, "uuid");
    proto_bug_read_string(decoder, token, sizeof token - 1, "token");
    proto_bug_read_string(decoder, code, sizeof code - 1, "code");
    if (code[0])
        snprintf(this->transfer_code, sizeof this->transfer_code, "%s", code);
    if (!ok || kind == rr_account_request_code_create || !uuid[0] || !token[0])
    {
        set_message(this, message);
        return;
    }
    // linked, or moved to another account: show it, then reconnect with it
    snprintf(this->auth_message, sizeof this->auth_message, "%s Reconnecting...",
             message);
    strcpy(this->rivet_account.uuid, uuid);
    strcpy(this->rivet_account.token, token);
    this->auth_reconnect_in = RECONNECT_DELAY;
    this->enter_code[0] = 0;
    rr_dom_set_text("_0x4348", this->enter_code);
    this->transfer_code[0] = 0;
    // if the page closes before the reconnect, the next visit still logs in
    rr_local_storage_store_string("rivet_account_uuid", uuid);
    rr_local_storage_store_string("DO_NOT_SHARE_account_token", token);
}

void rr_discord_oauth2_read_data(void *_this, void *_decoder) {
    struct rr_game *this = _this;
    struct proto_bug *decoder = _decoder;
    proto_bug_read_string(decoder, this->rivet_account.uuid,
                          sizeof this->rivet_account.uuid, "uuid");
    proto_bug_read_string(decoder, this->rivet_account.token,
                          sizeof this->rivet_account.token, "token");
    proto_bug_read_string(decoder, this->rivet_account.name,
                          sizeof this->rivet_account.name, "name");
    this->cg_linked = proto_bug_read_uint8(decoder, "crazygames linked");
    this->account_ready = 1;
    this->rivet_account.code[0] = 0;
    this->account_linked = this->rivet_account.name[0] != 0;
    if (!this->account_linked)
        strcpy(this->rivet_account.name, "Guest");
    rr_local_storage_store_string("rivet_account_uuid", this->rivet_account.uuid);
    rr_local_storage_store_string("DO_NOT_SHARE_account_token", this->rivet_account.token);
}

// Opens the Discord login in a popup. Discord redirects the popup back to the
// site (see public/index.html), which hands the code to /oauth/<state>, and
// this window polls that for the code. The relay is what lets it work when the
// game runs inside another site's iframe (CrazyGames).
#ifdef PDEV
void rr_discord_oauth2_link_account(void *captures) {
    EM_ASM({
        const origin = "https://game.parshwa.blog";
        const popupState = "p." + crypto.randomUUID();
        const url = new URL("https://discord.com/oauth2/authorize");
        url.searchParams.set("client_id", "1242286155320393859");
        url.searchParams.set("response_type", "code");
        url.searchParams.set("redirect_uri", origin + "/");
        url.searchParams.set("scope", "identify guilds.join");
        url.searchParams.set("state", popupState);
        const left = (window.screenX || 0) + (window.outerWidth - 500) / 2;
        const top = (window.screenY || 0) + (window.outerHeight - 800) / 2;
        const popup = window.open(url, "rrolf-oauth",
            "popup=yes,width=500,height=800,left=" + Math.max(0, left) + ",top=" + Math.max(0, top));
        if (!popup) {
            // popups blocked: log in on this page instead, which only works
            // when it is the page Discord redirects back to
            if (window.location.origin !== origin) {
                alert("Allow popups for this site to log in with Discord.");
                return;
            }
            const state = crypto.randomUUID();
            url.searchParams.set("state", state);
            window.localStorage["oauth2_state"] = state;
            window.onbeforeunload = null;
            window.location.href = url;
            return;
        }
        // only the newest attempt keeps polling
        Module.oauthState = popupState;
        const started = Date.now();
        const poll = function() {
            if (Module.oauthState !== popupState || Date.now() - started > 10 * 60 * 1000)
                return;
            fetch(origin + "/oauth/" + popupState).then(function(response) {
                if (response.status === 204) {
                    setTimeout(poll, 1000);
                    return;
                }
                return response.text().then(function(code) {
                    Module.oauthState = null;
                    if (response.ok && code)
                        Module.oauthLogIn(window.localStorage["rivet_account_uuid"] || "",
                            window.localStorage["DO_NOT_SHARE_account_token"] || "", code);
                });
            }).catch(function() {
                setTimeout(poll, 2000);
            });
        };
        poll();
    }, captures);
}

#else
void rr_discord_oauth2_link_account(void *captures) {
    EM_ASM({
        const origin = "https://rysteria.pro";
        const popupState = "p." + crypto.randomUUID();
        const url = new URL("https://discord.com/oauth2/authorize");
        url.searchParams.set("client_id", "1453525695228678349");
        url.searchParams.set("response_type", "code");
        url.searchParams.set("redirect_uri", origin + "/");
        url.searchParams.set("scope", "identify guilds.join");
        url.searchParams.set("state", popupState);
        const left = (window.screenX || 0) + (window.outerWidth - 500) / 2;
        const top = (window.screenY || 0) + (window.outerHeight - 800) / 2;
        const popup = window.open(url, "rrolf-oauth",
            "popup=yes,width=500,height=800,left=" + Math.max(0, left) + ",top=" + Math.max(0, top));
        if (!popup) {
            // popups blocked: log in on this page instead, which only works
            // when it is the page Discord redirects back to
            if (window.location.origin !== origin) {
                alert("Allow popups for this site to log in with Discord.");
                return;
            }
            const state = crypto.randomUUID();
            url.searchParams.set("state", state);
            window.localStorage["oauth2_state"] = state;
            window.onbeforeunload = null;
            window.location.href = url;
            return;
        }
        // only the newest attempt keeps polling
        Module.oauthState = popupState;
        const started = Date.now();
        const poll = function() {
            if (Module.oauthState !== popupState || Date.now() - started > 10 * 60 * 1000)
                return;
            fetch(origin + "/oauth/" + popupState).then(function(response) {
                if (response.status === 204) {
                    setTimeout(poll, 1000);
                    return;
                }
                return response.text().then(function(code) {
                    Module.oauthState = null;
                    if (response.ok && code)
                        Module.oauthLogIn(window.localStorage["rivet_account_uuid"] || "",
                            window.localStorage["DO_NOT_SHARE_account_token"] || "", code);
                });
            }).catch(function() {
                setTimeout(poll, 2000);
            });
        };
        poll();
    }, captures);
}

#endif

void rr_discord_oauth2_on_log_in(char *uuid, char *token, char *code, void *captures) {
    struct rr_game *this = captures;
    // the account is the one in this browser's storage. The first call of a
    // page load connects with it, a Discord login that follows later (the
    // popup) is sent over the connection that is already up
    if (!this->socket_ready && !this->socket_pending &&
        strlen(uuid) < sizeof this->rivet_account.uuid &&
        strlen(token) < sizeof this->rivet_account.token)
    {
        strcpy(this->rivet_account.uuid, uuid);
        strcpy(this->rivet_account.token, token);
    }
    // the code is exchanged for the Discord account at the master server,
    // through the game server, so the game server never sees more than that
    if (code[0] != 0)
        rr_auth_request(this, rr_account_request_discord, code);
    free(uuid);
    free(token);
    free(code);
    if (!this->socket_ready && !this->socket_pending)
        rr_game_connect_socket(this);
}

void rr_discord_oauth2_init(void *captures) {
    EM_ASM({
        const url = new URL(window.location);
        const uuid = url.searchParams.get("uuid") || window.localStorage["rivet_account_uuid"] || "";
        const token = window.localStorage["DO_NOT_SHARE_account_token"] || "";
        const _code = url.searchParams.get("code") || "";
        const _state = url.searchParams.get("state") || "";
        const state = window.localStorage["oauth2_state"];
        const code = (state && _state && state === _state) ? _code : "";
        url.searchParams.delete("uuid");
        url.searchParams.delete("code");
        url.searchParams.delete("state");
        url.searchParams.delete("error");
        url.searchParams.delete("error_description");
        window.history.replaceState(null, null, url);
        delete window.localStorage["oauth2_state"];
        // also used by the login popup (rr_discord_oauth2_link_account)
        Module.oauthLogIn = function(uuid, token, code) {
            const $uuid = _malloc(uuid.length + 1);
            const $token = _malloc(token.length + 1);
            const $code = _malloc(code.length + 1);
            HEAPU8.set(new TextEncoder().encode(uuid), $uuid);
            HEAPU8.set(new TextEncoder().encode(token), $token);
            HEAPU8.set(new TextEncoder().encode(code), $code);
            HEAPU8[$uuid + uuid.length] = 0;
            HEAPU8[$token + token.length] = 0;
            HEAPU8[$code + code.length] = 0;
            _rr_discord_oauth2_on_log_in($uuid, $token, $code, $0);
        };
        Module.oauthLogIn(uuid, token, code);
    }, captures);
}
