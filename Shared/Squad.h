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

#pragma once

#include <stdint.h>

#include <Shared/Entity.h>
#include <Shared/Utilities.h>

RR_SERVER_ONLY(struct rr_server_client;)
RR_CLIENT_ONLY(struct rr_ui_element;)

struct rr_squad_member
{
    RR_SERVER_ONLY(struct rr_server_client *client;)
    RR_CLIENT_ONLY(struct rr_ui_element *tooltip;)
    RR_CLIENT_ONLY(struct rr_ui_element *kick_text_el;)
    uint32_t level;
    uint8_t kick_vote_count;
    int8_t kick_vote_pos;
    uint8_t in_use : 1;
    uint8_t playing : 1;
    RR_CLIENT_ONLY(uint8_t disconnected : 1;)
    RR_CLIENT_ONLY(uint8_t blocked : 1;)
    uint8_t is_dev : 1;
    struct rr_id_rarity_pair loadout[RR_MAX_SLOT_COUNT * 2];
    char nickname[16];
    RR_CLIENT_ONLY(char uuid[37];)
    RR_CLIENT_ONLY(char discord[20];)
    RR_CLIENT_ONLY(char level_text[16];)
    RR_CLIENT_ONLY(char health_text[16];)
    RR_CLIENT_ONLY(char damage_text[16];)
    RR_CLIENT_ONLY(char kick_text[16];)
};