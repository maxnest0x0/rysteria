#pragma once

#include <stdint.h>

struct rr_game;

uint8_t rr_platform_on_crazygames(void);

uint8_t rr_platform_chat_disabled(void);

void rr_platform_tick(struct rr_game *game, float delta);

void rr_platform_copy_invite_link(char const *squad_code);

void rr_crazygames_login(void);

void rr_crazygames_link(void *game);

void rr_crazygames_on_link(char *cg_token, void *game);

void rr_auth_request(void *game, uint8_t kind, char const *payload);


void rr_auth_tick(struct rr_game *game, float delta);

void rr_auth_read_reply(void *game, void *decoder);

void rr_auth_get_transfer_code(void *game);
void rr_auth_use_transfer_code(void *game);
