#include <Server/Client.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Server/Server.h>
#include <Server/Simulation.h>
#include <Shared/Component/PlayerInfo.h>
#include <Shared/Crypto.h>
#include <Shared/Entity.h>

void rr_server_client_init(struct rr_server_client *this)
{
    memset(this, 0, sizeof *this);
    this->clientbound_encryption_key = rr_get_rand();
    this->serverbound_encryption_key = rr_get_rand();
    this->requested_verification = rr_get_rand();
    printf("client connected, using encryption data %lu %lu %lu\n",
           this->requested_verification, this->clientbound_encryption_key,
           this->serverbound_encryption_key);
}

void rr_server_client_create_flower(struct rr_server_client *this)
{
    if (this->player_info == NULL)
    {
        puts("tried to init a flower when player info is null");
        return;
    }
    if (this->player_info->flower_id != RR_NULL_ENTITY)
    {
        puts("tried to init a flower but flower already exists");
        return;
    }
    puts("creating flower");
    EntityIdx flower_id = rr_simulation_alloc_entity(&this->server->simulation);
    struct rr_component_physical *physical =
        rr_simulation_add_physical(&this->server->simulation, flower_id);
    struct rr_component_health *health =
        rr_simulation_add_health(&this->server->simulation, flower_id);
    struct rr_component_relations *relations =
        rr_simulation_add_relations(&this->server->simulation, flower_id);
    float distance =
        sqrt((float)rand() / (float)RAND_MAX) *
        rr_simulation_get_arena(&this->server->simulation, 1)->radius;
    float angle = (float)rand() / (float)RAND_MAX * M_PI * 2.0f;
    rr_component_physical_set_x(physical, cos(angle) * distance);
    rr_component_physical_set_y(physical, sin(angle) * distance);
    rr_component_physical_set_radius(physical, 25.0f);
    physical->friction = 0.9;
    if (rand() < RAND_MAX / 1000)
        rr_component_physical_set_angle(physical, rr_frand() * M_PI * 2);

    rr_simulation_add_flower(&this->server->simulation, flower_id);
    rr_component_health_set_max_health(health, 99999999100);
    rr_component_health_set_health(health, 19999900);
    health->damage = 10;
    rr_component_relations_set_team(relations, rr_simulation_team_id_players);
    rr_component_relations_set_owner(relations, this->player_info->parent_id);
    rr_component_player_info_set_camera_x(this->player_info, physical->x);
    rr_component_player_info_set_camera_y(this->player_info, physical->y);
    rr_component_player_info_set_flower_id(this->player_info, flower_id);
}
