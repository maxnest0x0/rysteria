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

#include <Shared/Component/Flower.h>

#include <stdio.h>
#include <string.h>

#include <Server/Server.h>
#include <Shared/Entity.h>
#include <Shared/SimulationCommon.h>
#include <Shared/pb.h>

#define FOR_EACH_PUBLIC_FIELD                                                  \
    X(eye_angle, float32)                                                      \
    X(face_flags, uint8)                                                       \
    X(level, varuint)                                                          \
    X(dead, uint8)                                                             \
    X(crest_count, uint8)                                                      \
    X(third_eye_count, uint8)

enum
{
    state_flags_face_flags = 0b000001,
    state_flags_level = 0b000010,
    state_flags_eye_angle = 0b000100,
    state_flags_nickname = 0b001000,
    state_flags_dead = 0b010000,
    state_flags_crest_count = 0b100000,
    state_flags_third_eye_count = 0b1000000,
    state_flags_all = 0b1111111
};

void rr_component_flower_init(struct rr_component_flower *this,
                              struct rr_simulation *simulation)
{
    memset(this, 0, sizeof *this);
}

#ifdef RR_SERVER
#include <math.h>

#include <Server/Client.h>
#include <Server/EntityAllocation.h>
#include <Server/Simulation.h>
#endif

void rr_component_flower_free(struct rr_component_flower *this,
                              struct rr_simulation *simulation)
{
#ifdef RR_SERVER
    if (rr_simulation_entity_alive(
            simulation,
            rr_simulation_get_relations(simulation, this->parent_id)->owner))
    {
        EntityHash p_id =
            rr_simulation_get_relations(simulation, this->parent_id)->owner;
        struct rr_component_player_info *player_info =
            rr_simulation_get_player_info(simulation, p_id);
        rr_component_player_info_set_flower_id(player_info, RR_NULL_ENTITY);
    }
#endif
}

#ifdef RR_SERVER
void rr_component_flower_set_dead(struct rr_component_flower *this,
                                  struct rr_simulation *simulation,
                                  uint8_t dead)
{
    this->protocol_state |= (this->dead != dead) * state_flags_dead;
    this->dead = dead;
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, this->parent_id);
    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(simulation, relations->owner);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, this->parent_id);
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, this->parent_id);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    if (dead)
    {
        player_info->input = 0;
        player_info->client->player_accel_x = 0;
        player_info->client->player_accel_y = 0;
        if (player_info->client->dev)
            rr_component_flower_set_face_flags(this, this->face_flags & ~3);
        else
            rr_component_flower_set_face_flags(this, this->face_flags | 1);
        rr_component_physical_set_angle(physical, 2 * M_PI * rr_frand());
        rr_component_health_set_health(health, 0);
        health->gradually_healed = 0;
        health->gradually_healed_ticks = 0;
        if (arena->pvp)
        {
            for (uint8_t squad = 0; squad < RR_SQUAD_COUNT; ++squad)
            {
                if (health->squad_damage_counter[squad] <=
                    health->max_health * 0.2)
                    continue;
                EntityIdx drop_id = rr_simulation_alloc_entity(simulation);
                struct rr_component_drop *drop =
                    rr_simulation_add_drop(simulation, drop_id);
                rr_component_drop_set_id(drop, rr_petal_id_basic);
                rr_component_drop_set_rarity(drop, rr_rarity_id_common);
                drop->ticks_until_despawn = 25 * 10 * (drop->rarity + 1);
                for (uint8_t pos = 0; pos < RR_SQUAD_MEMBER_COUNT; ++pos)
                {
                    struct rr_squad_member *member =
                        &simulation->server->squads[squad].members[pos];
                    if (member->in_use == 0)
                        continue;
                    uint8_t i = member->client - simulation->server->clients;
                    rr_bitset_set(drop->can_be_picked_up_by, i);
                }
                struct rr_component_physical *drop_physical =
                    rr_simulation_add_physical(simulation, drop_id);
                rr_component_physical_set_x(drop_physical, physical->x);
                rr_component_physical_set_y(drop_physical, physical->y);
                rr_component_physical_set_radius(drop_physical, 20);
                drop_physical->arena = physical->arena;
                struct rr_component_relations *drop_relations =
                    rr_simulation_add_relations(simulation, drop_id);
                rr_component_relations_set_team(drop_relations,
                                                rr_simulation_team_id_players);
            }
        }
    }
    else
    {
        for (uint8_t outer = 0; outer < player_info->slot_count; ++outer)
            for (uint8_t inner = 0; inner < player_info->slots[outer].count;
                 ++inner)
                player_info->slots[outer].petals[inner].cooldown_ticks =
                    RR_PETAL_DATA[player_info->slots[outer].id].cooldown;
        rr_component_physical_set_angle(physical, this->saved_angle);
        health->damage_paused = 63;
        health->health = 1;
        if (dev_cheat_enabled(simulation, this->parent_id, invulnerable))
            rr_component_health_set_health(health, health->max_health);
        else
            rr_component_health_set_health(health, 0.1 * health->max_health);
    }
}

void rr_component_flower_write(struct rr_component_flower *this,
                               struct proto_bug *encoder, int is_creation,
                               struct rr_component_player_info *client)
{
    uint64_t state = this->protocol_state | (state_flags_all * is_creation);
    proto_bug_write_varuint(encoder, state, "flower component state");
#define X(NAME, TYPE) RR_ENCODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
    if (state & state_flags_nickname)
        proto_bug_write_string(encoder, this->nickname, 16, "nickname");
}

RR_DEFINE_PUBLIC_FIELD(flower, uint8_t, face_flags)
RR_DEFINE_PUBLIC_FIELD(flower, float, eye_angle)
RR_DEFINE_PUBLIC_FIELD(flower, uint32_t, level)
RR_DEFINE_PUBLIC_FIELD(flower, uint8_t, crest_count)
RR_DEFINE_PUBLIC_FIELD(flower, uint8_t, third_eye_count)
#endif

#ifdef RR_CLIENT
void rr_component_flower_read(struct rr_component_flower *this,
                              struct proto_bug *encoder)
{
    uint64_t state = proto_bug_read_varuint(encoder, "flower component state");
#define X(NAME, TYPE) RR_DECODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
    if (state & state_flags_nickname)
        proto_bug_read_string(encoder, this->nickname, 16, "nickname");
}
#endif
