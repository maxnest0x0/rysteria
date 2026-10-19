#include <Client/Ui/Ui.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Platform.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Storage.h>
#include <Client/Ui/Engine.h>
#include <Shared/StaticData.h>

// The first-time tutorial. A banner at the top of the screen explains the
// current stage and a pulsing ring points at the button it is about:
//
//   title screen   welcome, Enter Game, squads, open the inventory, equip
//   arena          move, attack, defend, loot, swap petals
//   title screen   once the player owns 5 of the same petal that is not the
//                  basic one: craft it, then the mob gallery, then log in
//                  (CrazyGames login on CrazyGames, Discord anywhere else)
//
// "Info" stages have a Next button, the others end when the player does what
// they ask. Nothing times out: a stage ends when it is done, with Next, or
// with Skip tutorial, which is always there. The stage is remembered in localStorage
// ("rr_tutorial_stage"), so closing the page does not restart it.

enum stage
{
    st_welcome,
    st_enter,
    st_squad,
    st_open_inventory,
    st_equip,
    st_play,
    st_move,
    st_attack,
    st_defend,
    st_loot,
    st_swap,
    st_ready,
    // nothing is shown until the player has something to craft
    st_wait_craft,
    st_open_crafting,
    st_craft,
    st_gallery,
    st_login,
    st_end
};

#define BANNER_W 620
#define BANNER_H 104

struct view
{
    char const *title;
    char body[200];
    // bit per enum rr_tutorial_target
    uint16_t highlight;
    // has a Next button
    uint8_t info;
};

static uint8_t is_arena(struct rr_game *game)
{
    return game->simulation_ready && !game->flower_dead &&
           game->player_info != NULL &&
           game->player_info->flower_id != RR_NULL_ENTITY;
}

static uint8_t loadout_empty(struct rr_game *game)
{
    for (uint8_t i = 0; i < RR_MAX_SLOT_COUNT * 2; ++i)
        if (game->cache.loadout[i].id != 0)
            return 0;
    return 1;
}

// Fills the empty slots from the inventory (the lowest petals first) when
// nothing is equipped at all. Petals can only be equipped on the title
// screen, and a new account starts with petals but nothing equipped.
static void auto_equip(struct rr_game *game)
{
    if (game->simulation_ready || !loadout_empty(game))
        return;
    uint8_t slot = 0;
    for (uint8_t id = 1; id < rr_petal_id_max; ++id)
        for (uint8_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
            for (uint32_t n = game->inventory[id][rarity];
                 n > 0 && slot < game->slots_unlocked; --n)
            {
                game->cache.loadout[slot].id = id;
                game->cache.loadout[slot].rarity = rarity;
                ++slot;
            }
}

void rr_tutorial_on_join(struct rr_game *game)
{
    auto_equip(game);
}

static void save_stage(struct rr_game *game)
{
    char text[8];
    snprintf(text, sizeof text, "%u", game->tutorial.stage);
    rr_local_storage_store_string("rr_tutorial_stage", text);
}

static void finish(struct rr_game *game)
{
    game->tutorial.stage = st_end;
    save_stage(game);
    // whoever skips is not going to equip anything by hand
    auto_equip(game);
}

static void go_to(struct rr_game *game, uint8_t stage)
{
    // phones have no swap key, that stage is left out
    if (game->is_mobile && stage == st_swap)
        stage = st_ready;
    game->tutorial.stage = stage;
    game->tutorial.progress = 0;
    game->tutorial.saw_craft = 0;
    if (stage >= st_end)
    {
        finish(game);
        return;
    }
    save_stage(game);
}

void rr_tutorial_init(struct rr_game *game)
{
    uint32_t length = rr_local_storage_get_string("rr_tutorial_stage");
    if (length == 0)
    {
        game->tutorial.stage = st_welcome;
        return;
    }
    if (length > 3)
        length = 3;
    char text[4] = {0};
    memcpy(text, storage_buf1, length);
    int stage = atoi(text);
    game->tutorial.stage = stage < 0 || stage > st_end ? st_end : stage;
}

struct rr_ui_element *rr_tutorial_target(struct rr_game *game, uint8_t target,
                                         struct rr_ui_element *element)
{
    game->tutorial.targets[target] = element;
    return element;
}

// The first petal the player owns five unequipped copies of that is not the
// basic petal, and is not already the highest rarity.
static uint8_t find_craftable(struct rr_game *game)
{
    for (uint8_t id = rr_petal_id_basic + 1; id < rr_petal_id_max; ++id)
        for (uint8_t rarity = 0; rarity < rr_rarity_id_max - 1; ++rarity)
            if (game->inventory[id][rarity] >=
                game->loadout_counts[id][rarity] + 5u)
            {
                game->tutorial.craft_id = id;
                game->tutorial.craft_rarity = rarity;
                return 1;
            }
    return 0;
}

static uint8_t login_done(struct rr_game *game)
{
    return rr_platform_on_crazygames() ? game->cg_linked
                                       : game->account_linked;
}

static uint16_t bit(uint8_t target)
{
    return 1 << target;
}

static void set(struct view *view, char const *title, char const *body)
{
    view->title = title;
    snprintf(view->body, sizeof view->body, "%s", body);
}

// What the banner says right now. Returns 0 when it should not be shown.
static uint8_t describe(struct rr_game *game, struct view *view)
{
    uint8_t stage = game->tutorial.stage;
    uint8_t menu = game->menu_open;
    uint8_t mobile = game->is_mobile;
    view->title = "";
    view->body[0] = 0;
    view->highlight = 0;
    view->info = 0;
    if (stage >= st_end || game->cache.hide_ui)
        return 0;

    uint8_t on_title = !game->simulation_ready;
    uint8_t in_arena = is_arena(game) && game->tutorial.has_start;
    switch (stage)
    {
    case st_welcome:
        set(view, "Welcome to Rysteria!",
            "Fight mobs, collect petals and grow stronger. Here is a quick "
            "tour of how everything works.");
        view->info = 1;
        return on_title;
    case st_enter:
        set(view, "Enter Game",
            "Type a nickname above, then press Enter Game to drop into the "
            "arena.");
        view->info = 1;
        view->highlight = bit(rr_tutorial_target_join);
        return on_title;
    case st_squad:
        set(view, "Squads",
            "Create Squad starts a group for your friends, Find Squad joins "
            "one with a code. Squad mates play in the same arena.");
        view->info = 1;
        view->highlight = bit(rr_tutorial_target_create_squad) |
                          bit(rr_tutorial_target_find_squad);
        return on_title;
    case st_open_inventory:
        set(view, "Inventory",
            mobile ? "Open your inventory to see the petals you own."
                   : "Open your inventory (Z) to see the petals you own.");
        view->highlight = bit(rr_tutorial_target_inventory);
        return on_title;
    case st_equip:
        set(view, "Equip petals",
            "Click a petal to put it in a slot, again to fill more slots. "
            "Petals in your slots are the ones you fight with. Click a slot "
            "to take one out.");
        return on_title;
    case st_play:
        if (menu == rr_game_menu_none)
        {
            set(view, "Ready to play", "Press Enter Game to drop into the arena.");
            view->highlight = bit(rr_tutorial_target_join);
        }
        else
            set(view, "Ready to play",
                "Close the inventory, then press Enter Game.");
        return on_title;
    case st_move:
        set(view, "Move",
            mobile ? "Drag the joystick on the left to move."
                   : "Move with W A S D or the arrow keys.");
        return in_arena;
    case st_attack:
        set(view, "Attack",
            mobile ? "Hold the attack button on the right to throw your "
                     "petals."
                   : "Hold left click or Space to throw your petals at "
                     "enemies.");
        view->info = mobile;
        return in_arena;
    case st_defend:
        set(view, "Defend",
            mobile ? "Hold the defend button on the right to pull your "
                     "petals in."
                   : "Hold right click or Shift to pull your petals in "
                     "close.");
        // phones cannot be checked for this, so they press Next
        view->info = mobile;
        return in_arena;
    case st_loot:
        set(view, "Collect loot",
            "Defeat mobs and walk over the petals they drop to collect them.");
        view->info = 1;
        return in_arena;
    case st_swap:
        set(view, "Swap petals",
            "Press X to swap your petals with your reserve row, or 1-9 for "
            "one slot.");
        return in_arena;
    case st_ready:
        set(view, "Nice work!",
            "Collect 5 of the same petal and you can craft them into a "
            "stronger one from the title screen.");
        view->info = 1;
        return in_arena;
    case st_wait_craft:
        return 0;
    case st_open_crafting:
    {
        uint8_t id = game->tutorial.craft_id;
        uint8_t rarity = game->tutorial.craft_rarity;
        char body[200];
        snprintf(body, sizeof body,
                 "You have %u %s %s. Open Crafting%s to merge five of them "
                 "into one of the next rarity.",
                 rr_game_get_adjusted_inventory_count(game, id, rarity),
                 RR_RARITY_NAMES[rarity], RR_PETAL_NAMES[id],
                 mobile ? "" : " (C)");
        set(view, "Time to craft!", body);
        view->highlight = bit(rr_tutorial_target_crafting);
        return on_title;
    }
    case st_craft:
    {
        char body[200];
        snprintf(body, sizeof body,
                 "Click your %s to add them (you need 5), then press Craft. "
                 "Crafting can fail, so try again.",
                 RR_PETAL_NAMES[game->tutorial.craft_id]);
        set(view, "Crafting", body);
        return on_title;
    }
    case st_gallery:
        set(view, "Mob Gallery",
            mobile ? "Open the Mob Gallery to see every mob you have met and "
                     "what it can drop."
                   : "Open the Mob Gallery (V) to see every mob you have met "
                     "and what it can drop.");
        view->highlight = bit(rr_tutorial_target_gallery);
        return on_title;
    case st_login:
        if (menu == rr_game_menu_inventory || menu == rr_game_menu_gallery ||
            menu == rr_game_menu_crafting)
            return 0;
        if (rr_platform_on_crazygames())
        {
            if (!game->cg_logged_in)
            {
                set(view, "Save your progress",
                    "Log in with CrazyGames so your account follows you "
                    "everywhere.");
                view->highlight = bit(rr_tutorial_target_cg_login);
            }
            else if (menu == rr_game_menu_account)
            {
                set(view, "Link your account",
                    "Press Link CrazyGames to keep this account.");
                view->highlight = bit(rr_tutorial_target_cg_link);
            }
            else
            {
                set(view, "Link your account",
                    "Open your Account and press Link CrazyGames to keep "
                    "your progress.");
                view->highlight = bit(rr_tutorial_target_account);
            }
        }
        else if (menu == rr_game_menu_account)
        {
            set(view, "Save your progress",
                "Press the link button to log in with Discord.");
            view->highlight = bit(rr_tutorial_target_discord_link);
        }
        else
        {
            set(view, "Save your progress",
                "Open your Account and log in with Discord to keep your "
                "progress on any device.");
            view->highlight = bit(rr_tutorial_target_account);
        }
        return on_title;
    }
    return 0;
}

static void advance(struct rr_game *game)
{
    go_to(game, game->tutorial.stage + 1);
}

void rr_tutorial_tick(struct rr_game *game, float delta)
{
    game->tutorial.pulse += delta;
    uint8_t stage = game->tutorial.stage;
    if (stage >= st_end)
        return;
    // somebody who already plays does not need a tour
    if (stage == st_welcome && game->account_ready && level_from_xp(game->cache.experience) >= 5)
    {
        finish(game);
        return;
    }
    uint8_t arena = is_arena(game);
    if (!arena)
        game->tutorial.has_start = 0;
    // pressing Enter Game early skips what is left of the title screen part
    if (arena && stage < st_move)
        go_to(game, st_move);
    stage = game->tutorial.stage;
    // the craft prompt comes as soon as there is something to craft
    if (!game->simulation_ready && stage >= st_move &&
        stage < st_open_crafting && find_craftable(game))
        go_to(game, st_open_crafting);
    stage = game->tutorial.stage;

    uint8_t menu = game->menu_open;
    switch (stage)
    {
    case st_open_inventory:
        if (menu == rr_game_menu_inventory)
            go_to(game, st_equip);
        return;
    case st_equip:
        if (!loadout_empty(game))
            go_to(game, st_play);
        else if (menu != rr_game_menu_inventory)
            go_to(game, st_open_inventory);
        return;
    case st_open_crafting:
        if (menu == rr_game_menu_crafting)
            go_to(game, st_craft);
        return;
    case st_craft:
        if (game->crafting_data.animation > 0)
            game->tutorial.saw_craft = 1;
        if (game->tutorial.saw_craft && game->crafting_data.animation == 0)
            go_to(game, st_gallery);
        else if (!game->tutorial.saw_craft && menu != rr_game_menu_crafting)
            go_to(game, st_open_crafting);
        return;
    case st_gallery:
        if (menu == rr_game_menu_gallery)
            go_to(game, st_login);
        return;
    case st_login:
        if (login_done(game))
            go_to(game, st_end);
        return;
    default:
        break;
    }
    if (stage < st_move || stage > st_ready)
        return;

    // the arena stages
    if (!arena || game->cache.hide_ui)
        return;
    struct rr_component_physical *physical = rr_simulation_get_physical(
        game->simulation, game->player_info->flower_id);
    if (!game->tutorial.has_start)
    {
        game->tutorial.has_start = 1;
        game->tutorial.start_x = physical->lerp_x;
        game->tutorial.start_y = physical->lerp_y;
    }
    // a text box being typed in is not the player playing
    if (game->text_input_focused)
        return;

    uint8_t complete = 0;
    switch (stage)
    {
    case st_move:
    {
        float dx = physical->lerp_x - game->tutorial.start_x;
        float dy = physical->lerp_y - game->tutorial.start_y;
        complete = dx * dx + dy * dy > 250 * 250;
        break;
    }
    case st_attack:
        if (game->input_data->mouse_buttons & 1 ||
            rr_bitset_get(game->input_data->keys_pressed, 32) ||
            game->cache.hold_attack)
            game->tutorial.progress += delta;
        complete = game->tutorial.progress > 0.6f;
        break;
    case st_defend:
        if (game->input_data->mouse_buttons & 2 ||
            rr_bitset_get(game->input_data->keys_pressed, 16) ||
            game->cache.hold_defense)
            game->tutorial.progress += delta;
        complete = game->tutorial.progress > 0.6f;
        break;
    case st_swap:
        complete = rr_bitset_get_bit(game->input_data->keys_pressed_this_tick,
                                     'X');
        for (uint32_t key = '1'; key <= '9'; ++key)
            complete |= rr_bitset_get_bit(
                game->input_data->keys_pressed_this_tick, key);
        break;
    default:
        break;
    }
    // no timeouts: a stage ends when it is done, or with Next / Skip
    if (complete)
        advance(game);
}

static uint8_t banner_should_show(struct rr_ui_element *this,
                                  struct rr_game *game)
{
    struct view view;
    return describe(game, &view);
}

// The Skip and Next buttons are drawn at the bottom corners of the banner.
#define BUTTON_W 120
#define BUTTON_H 22

static void banner_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    struct view view;
    if (!describe(game, &view))
        return;
    float scale = game->renderer->scale;
    float x = (game->input_data->mouse_x - this->abs_x) / scale;
    float y = (game->input_data->mouse_y - this->abs_y) / scale;
    float top = BANNER_H / 2 - BUTTON_H - 12;
    uint8_t on_skip = y > top && x > -BANNER_W / 2 &&
                      x < -BANNER_W / 2 + BUTTON_W + 14;
    uint8_t on_next = view.info && y > top && x < BANNER_W / 2 &&
                      x > BANNER_W / 2 - BUTTON_W - 14;
    if (on_skip || on_next)
        game->cursor = rr_game_cursor_pointer;
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (on_skip)
            finish(game);
        else if (on_next)
            advance(game);
    }
}

// splits text into two lines at a space
static void wrap(char const *text, char *first, char *second, size_t size)
{
    size_t const limit = 66;
    size_t length = strlen(text);
    if (length <= limit)
    {
        snprintf(first, size, "%s", text);
        second[0] = 0;
        return;
    }
    size_t cut = limit;
    while (cut > 0 && text[cut] != ' ')
        --cut;
    if (cut == 0)
        cut = limit;
    snprintf(first, size, "%.*s", (int)cut, text);
    snprintf(second, size, "%s", text + cut + (text[cut] == ' '));
}

static void draw_text(struct rr_renderer *renderer, char const *text,
                      float size, uint32_t fill, float x, float y)
{
    rr_renderer_set_fill(renderer, fill);
    rr_renderer_set_text_size(renderer, size);
    rr_renderer_set_line_width(renderer, size * 0.12);
    rr_renderer_stroke_text(renderer, text, x, y);
    rr_renderer_fill_text(renderer, text, x, y);
}

static void draw_button(struct rr_renderer *renderer, char const *label,
                        uint32_t fill, float x, float y)
{
    rr_renderer_set_fill(renderer, fill);
    rr_renderer_begin_path(renderer);
    rr_renderer_round_rect(renderer, x - BUTTON_W / 2, y - BUTTON_H / 2,
                           BUTTON_W, BUTTON_H, 6);
    rr_renderer_fill(renderer);
    draw_text(renderer, label, 14, 0xffffffff, x, y);
}

static void banner_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct view view;
    if (!describe(game, &view))
        return;
    struct rr_renderer *renderer = game->renderer;
    char first[200];
    char second[200];
    wrap(view.body, first, second, sizeof first);
    rr_renderer_scale(renderer, renderer->scale);
    rr_renderer_set_fill(renderer, 0xb0000000);
    rr_renderer_begin_path(renderer);
    rr_renderer_round_rect(renderer, -this->width / 2, -this->height / 2,
                           this->width, this->height, 8);
    rr_renderer_fill(renderer);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_text_align(renderer, 1);
    rr_renderer_set_stroke(renderer, 0xff222222);
    draw_text(renderer, view.title, 20, 0xffffe763, 0, -34);
    draw_text(renderer, first, 15, 0xffffffff, 0, -10);
    if (second[0])
        draw_text(renderer, second, 15, 0xffffffff, 0, 9);
    float y = BANNER_H / 2 - BUTTON_H / 2 - 8;
    draw_button(renderer, "Skip tutorial", 0xff666666,
                -BANNER_W / 2 + BUTTON_W / 2 + 10, y);
    if (view.info)
        draw_button(renderer, "Next", 0xffd4a017,
                    BANNER_W / 2 - BUTTON_W / 2 - 10, y);
}

struct rr_ui_element *rr_ui_tutorial_init(struct rr_game *game)
{
    struct rr_ui_element *this = rr_ui_element_init();
    this->abs_width = this->width = BANNER_W;
    this->abs_height = this->height = BANNER_H;
    this->on_event = banner_on_event;
    this->on_render = banner_on_render;
    this->should_show = banner_should_show;
    return rr_ui_link_toggle(
        rr_ui_v_pad(rr_ui_set_justify(
                        rr_ui_v_container_init(rr_ui_container_init(), 0, 0,
                                               this, NULL),
                        0, -1),
                    70),
        banner_should_show);
}

// Draws the ring in screen pixels, with the position the buttons were last
// drawn at, so it has to be the last element to render.
static void highlight_on_render(struct rr_ui_element *this,
                                struct rr_game *game)
{
    struct view view;
    if (!describe(game, &view) || view.highlight == 0)
        return;
    struct rr_renderer *renderer = game->renderer;
    float scale = renderer->scale;
    float pulse = 0.5f + 0.5f * sinf(game->tutorial.pulse * 6);
    rr_renderer_set_transform(renderer, 1, 0, 0, 0, 1, 0);
    rr_renderer_set_global_alpha(renderer, 1);
    rr_renderer_set_stroke(renderer, 0xffffe763);
    rr_renderer_set_line_width(renderer, 4 * scale);
    for (uint8_t i = 0; i < rr_tutorial_target_max; ++i)
    {
        struct rr_ui_element *target = game->tutorial.targets[i];
        if (!(view.highlight & bit(i)) || target == NULL ||
            target->completely_hidden || target->abs_width <= 0)
            continue;
        float pad = (6 + 5 * pulse) * scale;
        float w = target->abs_width * scale + 2 * pad;
        float h = target->abs_height * scale + 2 * pad;
        rr_renderer_begin_path(renderer);
        rr_renderer_round_rect(renderer, target->abs_x - w / 2,
                               target->abs_y - h / 2, w, h, 10 * scale);
        rr_renderer_stroke(renderer);
    }
}

struct rr_ui_element *rr_ui_tutorial_highlight_init(struct rr_game *game)
{
    struct rr_ui_element *this = rr_ui_element_init();
    this->abs_width = this->width = 1;
    this->abs_height = this->height = 1;
    this->on_render = highlight_on_render;
    this->should_show = rr_ui_always_show;
    this->prevent_on_event = 1;
    return this;
}
