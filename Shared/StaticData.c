#include <Shared/StaticData.h>

struct rr_petal_data RR_PETAL_DATA[rr_petal_id_max] = {
    {rr_petal_id_no_petal, 0.0f,   0.0f,  0.0f,  0,    {          }},
    {rr_petal_id_basic,    10.0f,  10.0f, 0.0f,  37,   {1, 1, 1, 1}},
    {rr_petal_id_light,    13.0f,  5.0f,  0.0f,  12,   {1, 2, 2, 3}},
    {rr_petal_id_stinger,  45.0f,  2.0f,  10.0f, 100,  {1, 1, 1, 3}},
    {rr_petal_id_faster,   10.0f,  5.0f,  25.0f, 25,   {1, 1, 1, 6}},
    {rr_petal_id_missile,  75.0f,  5.0f,  15.0f, 100,  {1, 1, 1, 3}},
};

struct rr_mob_data RR_MOB_DATA[rr_mob_id_max] = {
    {rr_mob_id_baby_ant,   1, 1, 17.5},
    {rr_mob_id_worker_ant, 1, 1, 17.5},
    {rr_mob_id_centipede_head, 10, 1, 35},
    {rr_mob_id_centipede_body, 10, 1, 35}
};

struct rr_mob_rarity_scale RR_MOB_RARITY_SCALING[rr_rarity_max] = {
    {1, 1, 1},
    {2, 2, 1.1},
    {6, 6, 1.3},
    {15, 15, 1.6}
};
double RR_DROP_RARITY_COEFFICIENTS[rr_rarity_max + 1] = {0, 1, 2.5, 10, 15};
static void init_rarity_coefficients()
{
    double sum = 1;
    for (uint64_t a = 2; a <= rr_rarity_max; ++a)
        sum += (RR_DROP_RARITY_COEFFICIENTS[a] = RR_DROP_RARITY_COEFFICIENTS[a] / RR_DROP_RARITY_COEFFICIENTS[a - 1]);
    for (uint64_t a = 1; a <= rr_rarity_max; ++a)
        RR_DROP_RARITY_COEFFICIENTS[a] = RR_DROP_RARITY_COEFFICIENTS[a] / sum + RR_DROP_RARITY_COEFFICIENTS[a - 1];
}

uint32_t RR_RARITY_COLORS[rr_rarity_max] = {0xff7eef6d, 0xffffe65d, 0xff4d52e3, 0xff861fde };
char const *RR_RARITY_NAMES[rr_rarity_max] = {"Common", "Unusual", "Rare", "Epic"};
