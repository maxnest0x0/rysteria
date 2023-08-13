#include <Server/System/CollisionDetection.h>

#include <stdio.h>
#include <string.h>

#include <Server/Simulation.h>
#include <Server/SpatialHash.h>
#include <Shared/Bitset.h>

struct physics_simulation_captures
{
    struct rr_simulation *simulation;
    struct rr_component_physical *physical;
};

static void system_reset_colliding_with(EntityIdx entity, void *captures)
{
    struct rr_simulation *this = captures;
    if (!rr_simulation_has_physical(this, entity))
        return;

    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    physical->colliding_with_size = 0;
}

static void system_insert_entities(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this, entity);
    if (rr_simulation_has_health(this, entity) &&
        rr_simulation_get_health(this, entity)->health == 0)
        return;
    rr_spatial_hash_insert(this->grid, entity);
}
static uint8_t should_entities_collide(struct rr_simulation *this, EntityIdx a,
                                       EntityIdx b)
{
#define exclude(component_a, component_b)                                      \
    if (rr_simulation_has_##component_a(this, a) &&                            \
        rr_simulation_has_##component_b(this, b))                              \
        return 0;                                                              \
    if (rr_simulation_has_##component_a(this, b) &&                            \
        rr_simulation_has_##component_b(this, a))                              \
        return 0;

    exclude(drop, drop);
    exclude(drop, mob);
    uint8_t team1 = rr_simulation_get_relations(this, a)->team;
    uint8_t team2 = rr_simulation_get_relations(this, b)->team;
    if (team1 != team2)
        return 1; // only drop doesn't care about team
    exclude(petal, petal);
    exclude(petal, flower);
    exclude(petal, mob);
    exclude(flower, mob);
#undef exclude

    return 1;
}
static void grid_filter_candidates(struct rr_simulation *this,
                                   EntityIdx entity1, EntityIdx entity2,
                                   void *_captures)
{
    struct rr_component_physical *physical1 =
        rr_simulation_get_physical(this, entity1);
    struct rr_component_physical *physical2 =
        rr_simulation_get_physical(this, entity2);
    struct rr_vector delta = {physical1->x - physical2->x,
                              physical1->y - physical2->y};
    float collision_radius = physical1->radius + physical2->radius;
    if ((delta.x * delta.x + delta.y * delta.y) <
        collision_radius * collision_radius)
    {
        if (!should_entities_collide(this, entity1, entity2))
            return;
        physical1->colliding_with[physical1->colliding_with_size++] = entity2;
#ifndef RIVET_BUILD
        if (physical1->colliding_with_size >= RR_MAX_COLLISION_COUNT)
            puts("entity cram limit exceeded");
#endif
    }
}

static void find_collisions(struct rr_simulation *this)
{
    rr_spatial_hash_find_possible_collisions(this->grid, NULL,
                                             grid_filter_candidates);
}

static void update_spatial_hash_entities(EntityIdx entity, void *_captures)
{
    struct rr_simulation *this = _captures;
    rr_spatial_hash_insert(this->grid, entity);
}

void rr_system_collision_detection_tick(struct rr_simulation *this)
{
    rr_spatial_hash_reset(this->grid);
    rr_simulation_for_each_physical(this, this, system_reset_colliding_with);
    rr_simulation_for_each_physical(this, this, system_insert_entities);
    // rr_simulation_for_each_physical(this, this,
    // update_spatial_hash_entities);
    find_collisions(this);
}
