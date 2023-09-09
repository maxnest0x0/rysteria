#include <Server/System/Drops.h>

#include <stdlib.h>

#include <Server/Simulation.h>

struct drop_pick_up_captures
{
    struct rr_simulation *simulation;
    EntityIdx self;
};

static void drop_pick_up(EntityIdx entity, void *_captures)
{
    struct drop_pick_up_captures *captures = _captures;
    struct rr_simulation *this = captures->simulation;
    EntityIdx drop_id = captures->self;
    struct rr_component_drop *drop = rr_simulation_get_drop(this, drop_id);

    struct rr_component_relations *flower_relations =
        rr_simulation_get_relations(this, entity);
    if (!rr_simulation_has_entity(this, flower_relations->owner))
        return;

    struct rr_component_player_info *player_info =
        rr_simulation_get_player_info(this, flower_relations->owner);
    if (!rr_bitset_get(drop->can_be_picked_up_by, player_info->squad))
        return;
    if (rr_bitset_get(drop->picked_up_by, player_info->squad * RR_SQUAD_MEMBER_COUNT + player_info->squad_pos))
        return;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, drop_id);

    struct rr_component_physical *flower_physical =
        rr_simulation_get_physical(this, entity);

    struct rr_vector delta = {physical->x - flower_physical->x,
                              physical->y - flower_physical->y};
    if (rr_vector_get_magnitude(&delta) >
        physical->radius + player_info->modifiers.drop_pickup_radius)
        return;
    if (player_info->drops_this_tick_size >= 8)
        return;
    rr_bitset_set(drop->picked_up_by, player_info->squad * RR_SQUAD_MEMBER_COUNT + player_info->squad_pos);

    ++player_info->collected_this_run[drop->id * rr_rarity_id_max + drop->rarity];
    rr_component_player_info_set_update_loot(player_info);
    player_info->drops_this_tick[player_info->drops_this_tick_size].id = drop->id;
    player_info->drops_this_tick[player_info->drops_this_tick_size].rarity = drop->rarity;
    ++player_info->drops_this_tick_size;
}

static void drop_despawn_tick(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_drop *drop = rr_simulation_get_drop(this, entity);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    if (drop->ticks_until_despawn == 0)
    {
        rr_simulation_request_entity_deletion(this, entity);
        return;
    }
    --drop->ticks_until_despawn;
    if (drop->ticks_until_despawn > 25 * 10 * (drop->rarity + 1) - 10)
        return;
    struct drop_pick_up_captures captures;
    captures.self = entity;
    captures.simulation = this;
    rr_simulation_for_each_flower(this, &captures, drop_pick_up);
}

void rr_system_drops_tick(struct rr_simulation *this)
{
    rr_simulation_for_each_drop(this, this, drop_despawn_tick);
}
