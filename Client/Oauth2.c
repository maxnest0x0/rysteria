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

#include <string.h>

#include <Client/Game.h>
#include <Client/Storage.h>

void rr_discord_oauth2_read_data(void *_this, void *_decoder) {
    struct rr_game *this = _this;
    struct proto_bug *decoder = _decoder;
    proto_bug_read_string(decoder, this->rivet_account.uuid,
                          sizeof this->rivet_account.uuid, "uuid");
    proto_bug_read_string(decoder, this->rivet_account.token,
                          sizeof this->rivet_account.token, "token");
    proto_bug_read_string(decoder, this->rivet_account.name,
                          sizeof this->rivet_account.name, "name");
    this->rivet_account.code[0] = 0;
    if (this->rivet_account.name[0])
        this->account_linked = 1;
    else
        strcpy(this->rivet_account.name, "Guest");
    rr_local_storage_store_string("rivet_account_uuid", this->rivet_account.uuid);
    rr_local_storage_store_string("DO_NOT_SHARE_account_token", this->rivet_account.token);
}

void rr_discord_oauth2_link_account() {
    EM_ASM({
        const state = crypto.randomUUID();
        const url = new URL("https://discord.com/oauth2/authorize");
        url.searchParams.set("client_id", "1453525695228678349");
        url.searchParams.set("response_type", "code");
        url.searchParams.set("redirect_uri", window.location.origin + window.location.pathname);
        url.searchParams.set("scope", "identify guilds.join");
        url.searchParams.set("state", state);
        window.localStorage["oauth2_state"] = state;
        window.onbeforeunload = null;
        window.location.href = url;
    });
}

void rr_discord_oauth2_on_log_in(char *uuid, char *token, char *code, void *captures) {
    struct rr_game *this = captures;
    strcpy(this->rivet_account.uuid, uuid);
    strcpy(this->rivet_account.token, token);
    strcpy(this->rivet_account.code, code);
    free(uuid);
    free(token);
    free(code);
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
    }, captures);
}
