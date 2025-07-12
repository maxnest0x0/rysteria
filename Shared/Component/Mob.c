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

#include <Shared/Component/Mob.h>

#include <string.h>

#include <Shared/Entity.h>
#include <Shared/SimulationCommon.h>
#include <Shared/pb.h>

#ifdef RR_SERVER
#include <math.h>
#include <stdio.h>

#include <Server/EntityAllocation.h>
#include <Server/Server.h>
#include <Server/Simulation.h>

#include <Shared/StaticData.h>
#include <Shared/Utilities.h>
#endif

#define FOR_EACH_PUBLIC_FIELD                                                  \
    X(id, uint8)                                                               \
    X(rarity, uint8)                                                           \
    X(player_spawned, uint8)

enum
{
    state_flags_rarity = 0b000001,
    state_flags_id = 0b000010,
    state_flags_player_spawned = 0b000100,
    state_flags_all = 0b000111
};

void rr_component_mob_init(struct rr_component_mob *this,
                           struct rr_simulation *simulation)
{
    memset(this, 0, sizeof *this);
    RR_SERVER_ONLY(this->ticks_to_despawn = 120 * 25;)
}

void rr_component_mob_free(struct rr_component_mob *this,
                           struct rr_simulation *simulation)
{
#ifdef RR_SERVER
    struct rr_component_relations *relations =
        rr_simulation_get_relations(simulation, this->parent_id);
    if (this->player_spawned)
    {
        EntityHash hash =
            rr_simulation_get_entity_hash(simulation, this->parent_id);
        for (uint32_t i = 0; i < simulation->ai_count; ++i)
        {
            struct rr_component_ai *ai =
                rr_simulation_get_ai(simulation, simulation->ai_vector[i]);
            if (ai->target_entity == hash)
                ai->target_entity = relations->owner;
        }
        return;
    }
    this->zone->grid_points -= RR_MOB_DIFFICULTY_COEFFICIENTS[this->id];
    // put it here please
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, this->parent_id);
    struct rr_component_health *health =
        rr_simulation_get_health(simulation, this->parent_id);
    struct rr_component_arena *arena =
        rr_simulation_get_arena(simulation, physical->arena);
    --arena->mob_count;
    if (this->no_drop)
        return;
    for (uint32_t squad = 0; squad < RR_SQUAD_COUNT; ++squad)
    {
        if (rr_simulation_has_arena(simulation, this->parent_id) &&
            rr_simulation_get_arena(simulation, this->parent_id)->player_entered)
        {
            if (squad != rr_simulation_get_arena(simulation, this->parent_id)
                             ->first_squad_to_enter)
                continue;
        }
        else if (this->id != rr_mob_id_meteor &&
                 health->squad_damage_counter[squad] <=
                     health->max_health * 0.2)
            continue;

        for (uint8_t pos = 0; pos < RR_SQUAD_MEMBER_COUNT; ++pos)
        {
            struct rr_squad_member *member =
                &simulation->server->squads[squad].members[pos];
            if (member->in_use == 0)
                continue;
            if (member->client->disconnected)
                continue;
            if (member->client->verified == 0)
                continue;
            if (member->client->player_info == NULL)
                continue;
            if (member->client->player_info->flower_id == RR_NULL_ENTITY)
                continue;
            struct rr_component_physical *flower_physical =
                rr_simulation_get_physical(
                    simulation, member->client->player_info->flower_id);
            struct rr_vector delta = {physical->x - flower_physical->x,
                                      physical->y - flower_physical->y};
            if (rr_vector_magnitude_cmp(&delta, 2000) == 1)
                continue;
            ++member->client->mob_gallery[this->id][this->rarity];
            rr_server_client_write_to_api(member->client);
            rr_server_client_write_account(member->client);
        }

        uint8_t spawn_ids[4] = {};
        uint8_t spawn_rarities[4] = {};
        uint8_t count = 0;

        for (uint64_t i = 0; i < 4; ++i)
        {
            if (RR_MOB_DATA[this->id].loot[i].id == 0)
                break;
            uint8_t id = RR_MOB_DATA[this->id].loot[i].id;
            float seed = rr_frand();
            float s2 = RR_MOB_DATA[this->id].loot[i].seed;
            uint8_t drop;
            uint8_t cap = this->rarity >= rr_rarity_id_exotic ? this->rarity - 1
                                                              : this->rarity;

            for (drop = 0; drop <= cap + 1; ++drop)
            {
                double end =
                    drop == cap + 1 ? 1 : RR_DROP_RARITY_COEFFICIENTS[drop];
                if (cap < RR_PETAL_DATA[id].min_rarity)
                    end = 1;
                else if (drop < RR_PETAL_DATA[id].min_rarity)
                    end = RR_DROP_RARITY_COEFFICIENTS[RR_PETAL_DATA[id].min_rarity];
                if (seed <= pow(1 - (1 - end) * s2,
                                RR_MOB_LOOT_RARITY_COEFFICIENTS[this->rarity]))
                    break;
            }
            if (drop == 0)
                continue;
            spawn_ids[count] = RR_MOB_DATA[this->id].loot[i].id;
            spawn_rarities[count] = drop - 1;
            ++count;
        }
        for (uint8_t i = 0; i < count; ++i)
        {
            EntityIdx entity = rr_simulation_alloc_entity(simulation);
            struct rr_component_physical *drop_physical =
                rr_simulation_add_physical(simulation, entity);
            struct rr_component_drop *drop =
                rr_simulation_add_drop(simulation, entity);
            struct rr_component_relations *relations =
                rr_simulation_add_relations(simulation, entity);
            rr_component_physical_set_x(drop_physical, physical->x);
            rr_component_physical_set_y(drop_physical, physical->y);
            rr_component_physical_set_radius(drop_physical, 20);

            rr_component_drop_set_id(drop, spawn_ids[i]);
            rr_component_drop_set_rarity(drop, spawn_rarities[i]);

            rr_component_relations_set_team(relations,
                                            rr_simulation_team_id_players);
            drop->ticks_until_despawn = 25 * 10 * (spawn_rarities[i] + 1);
            for (uint8_t pos = 0; pos < RR_SQUAD_MEMBER_COUNT; ++pos)
            {
                struct rr_squad_member *member =
                    &simulation->server->squads[squad].members[pos];
                if (member->in_use == 0)
                    continue;
                uint8_t j = member->client - simulation->server->clients;
                rr_bitset_set(drop->can_be_picked_up_by, j);
            }
            drop_physical->arena = physical->arena;
            if (count != 1)
            {
                float angle = M_PI * 2 * (i + 0.65 * rr_frand()) / count;
                rr_vector_from_polar(&drop_physical->velocity, 15 + 20 * rr_frand(), angle);
                drop_physical->friction = 0.75;
            }
        }
    }
#endif
}

#ifdef RR_SERVER
void rr_component_mob_write(struct rr_component_mob *this,
                            struct proto_bug *encoder, int is_creation,
                            struct rr_component_player_info *client)
{
    uint64_t state = this->protocol_state | (state_flags_all * is_creation);
    proto_bug_write_varuint(encoder, state, "mob component state");
#define X(NAME, TYPE) RR_ENCODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
}

RR_DEFINE_PUBLIC_FIELD(mob, uint8_t, id)
RR_DEFINE_PUBLIC_FIELD(mob, uint8_t, rarity)
RR_DEFINE_PUBLIC_FIELD(mob, uint8_t, player_spawned)
#endif

#ifdef RR_CLIENT
void rr_component_mob_read(struct rr_component_mob *this,
                           struct proto_bug *encoder)
{
    uint64_t state = proto_bug_read_varuint(encoder, "mob component state");
#define X(NAME, TYPE) RR_DECODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
}
#endif