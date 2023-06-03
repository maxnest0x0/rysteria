#include <Shared/Component/Petal.h>

#include <string.h>
#include <stdlib.h>

#include <Shared/pb.h>
#include <Shared/Entity.h>
#include <Shared/SimulationCommon.h>

enum
{
    state_flags_id =     0b000001,
    state_flags_rarity = 0b000010,
    state_flags_all =    0b000011
};

void rr_component_petal_init(struct rr_component_petal *this)
{
    memset(this, 0, sizeof *this);
    RR_SERVER_ONLY(this->spin_ccw = 1 - 2 * (rand() & 1);)
}

void rr_component_petal_free(struct rr_component_petal *this, struct rr_simulation *simulation)
{
#ifdef RR_SERVER
    if (this->detached)
        return;
    if (!rr_simulation_has_entity(simulation, this->player_info))
        return; // stop check discord
    if (!rr_simulation_has_player_info(simulation, this->player_info))
        return;
    struct rr_component_player_info *player_info = rr_simulation_get_player_info(simulation, this->player_info);
    struct rr_component_player_info_petal *ppetal = &player_info->slots[this->outer_pos].petals[this->inner_pos];
    if (ppetal->simulation_id != this->parent_id) 
        return;
    ppetal->simulation_id = RR_NULL_ENTITY;
    ppetal->cooldown_ticks = player_info->slots[this->outer_pos].data->cooldown;
#endif
}

#ifdef RR_SERVER
void rr_component_petal_write(struct rr_component_petal *this, struct proto_bug *encoder, int is_creation)
{
    uint64_t state = this->protocol_state | (state_flags_all * is_creation);
    proto_bug_write_varuint(encoder, state, "petal component state");
    RR_ENCODE_PUBLIC_FIELD(id, uint8);
    RR_ENCODE_PUBLIC_FIELD(rarity, uint8);
}

RR_DEFINE_PUBLIC_FIELD(petal, uint8_t, id)
RR_DEFINE_PUBLIC_FIELD(petal, uint8_t, rarity)
#endif

#ifdef RR_CLIENT
void rr_component_petal_read(struct rr_component_petal *this, struct proto_bug *encoder)
{
    uint64_t state = proto_bug_read_varuint(encoder, "petal component state");
    RR_DECODE_PUBLIC_FIELD(id, uint8);
    RR_DECODE_PUBLIC_FIELD(rarity, uint8);
}
#endif