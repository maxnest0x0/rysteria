#include <Client/Ui/Ui.h>

#include <stdio.h>
#include <stdlib.h>

#include <Client/Game.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Ui/Engine.h>

#include <Shared/Utilities.h>

struct leaderboard_row_metadata
{
    uint8_t rank;
};

static void leaderboard_row_draw(struct rr_ui_element *this,
                                 struct rr_renderer *renderer,
                                 uint32_t rank, char const *nickname,
                                 uint64_t points, uint32_t fill)
{
    rr_renderer_scale(renderer, renderer->scale);
    rr_renderer_translate(renderer, -this->abs_width / 2, 0);
    rr_renderer_set_fill(renderer, fill);
    rr_renderer_set_stroke(renderer, 0xff222222);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_text_size(renderer, 16);
    rr_renderer_set_line_width(renderer, 16 * 0.12);

    char rank_text[12];
    snprintf(rank_text, sizeof rank_text, "#%u", rank);
    rr_renderer_set_text_align(renderer, 0);
    rr_renderer_stroke_text(renderer, rank_text, 0, 0);
    rr_renderer_fill_text(renderer, rank_text, 0, 0);

    rr_renderer_stroke_text(renderer, nickname, 35, 0);
    rr_renderer_fill_text(renderer, nickname, 35, 0);

    char points_text[16];
    rr_sprintf(points_text, (double)points);
    rr_renderer_set_text_align(renderer, 2);
    rr_renderer_stroke_text(renderer, points_text, this->abs_width, 0);
    rr_renderer_fill_text(renderer, points_text, this->abs_width, 0);
}

static uint8_t leaderboard_row_should_show(struct rr_ui_element *this,
                                           struct rr_game *game)
{
    struct leaderboard_row_metadata *data = this->data;
    return data->rank < game->leaderboard_count;
}

static void leaderboard_row_on_render(struct rr_ui_element *this,
                                      struct rr_game *game)
{
    struct leaderboard_row_metadata *data = this->data;
    struct rr_leaderboard_entry *entry = &game->leaderboard[data->rank];
    leaderboard_row_draw(this, game->renderer, data->rank + 1, entry->nickname,
                         entry->points, game->leaderboard_own_rank ==
                                        data->rank + 1 ? 0xffdde27a
                                                       : 0xffffffff);
}

static struct rr_ui_element *leaderboard_row_init(uint8_t rank)
{
    struct rr_ui_element *this = rr_ui_element_init();
    struct leaderboard_row_metadata *data = malloc(sizeof *data);
    data->rank = rank;
    this->data = data;
    this->abs_width = this->width = 220;
    this->abs_height = this->height = 22;
    this->on_render = leaderboard_row_on_render;
    this->should_show = leaderboard_row_should_show;
    return this;
}

static uint8_t leaderboard_own_row_should_show(struct rr_ui_element *this,
                                               struct rr_game *game)
{
    return game->leaderboard_own_rank > game->leaderboard_count;
}

static void leaderboard_own_row_on_render(struct rr_ui_element *this,
                                          struct rr_game *game)
{
    leaderboard_row_draw(this, game->renderer, game->leaderboard_own_rank,
                         game->squad.squad_members
                             [game->player_info->squad_pos].nickname,
                         game->leaderboard_own_points, 0xffdde27a);
}

static struct rr_ui_element *leaderboard_own_row_init()
{
    struct rr_ui_element *this = rr_ui_element_init();
    this->abs_width = this->width = 220;
    this->abs_height = this->height = 22;
    this->on_render = leaderboard_own_row_on_render;
    this->should_show = leaderboard_own_row_should_show;
    return this;
}

static uint8_t leaderboard_should_show(struct rr_ui_element *this,
                                       struct rr_game *game)
{
    return game->selected_biome == rr_biome_id_pvp &&
           game->leaderboard_count > 0;
}

struct rr_ui_element *rr_ui_leaderboard_init(struct rr_game *game)
{
    struct rr_ui_element *rows = rr_ui_v_container_init(
        rr_ui_container_init(), 0, 3, leaderboard_row_init(0),
        leaderboard_row_init(1), leaderboard_row_init(2),
        leaderboard_row_init(3), leaderboard_row_init(4),
        leaderboard_row_init(5), leaderboard_row_init(6),
        leaderboard_row_init(7), leaderboard_row_init(8),
        leaderboard_row_init(9), leaderboard_own_row_init(), NULL);
    return rr_ui_link_toggle(
        rr_ui_set_justify(
            rr_ui_v_container_init(
                rr_ui_container_init(), 10, 10,
                rr_ui_text_init("Leaderboard", 20, 0xffffffff), rows, NULL),
            1, -1),
        leaderboard_should_show);
}
