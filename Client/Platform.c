#include <Client/Platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Client/DOM.h>
#include <Client/Game.h>
#include <Client/Socket.h>
#include <Shared/StaticData.h>
#include <Shared/pb.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

uint8_t rr_platform_on_crazygames(void)
{
#ifdef __EMSCRIPTEN__
    // the mode is fixed before the game starts
    static int8_t cached = -1;
    if (cached < 0)
        cached = EM_ASM_INT({
            const platform = window["rrolfPlatform"];
            return +!!platform && (platform["mode"] === "crazygames" ||
                                   platform["mode"] === "local");
        });
    return cached;
#else
    return 0;
#endif
}

uint8_t rr_platform_chat_disabled(void)
{
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        const platform = window["rrolfPlatform"];
        return +!!platform && !!platform["disableChat"];
    });
#else
    return 0;
#endif
}

#ifdef __EMSCRIPTEN__
// Gets a user token from platform.js (which logs the player in first if
// needed) and hands it to rr_crazygames_on_link. after_prompt asks CrazyGames'
// own "link your account?" question first and only goes on if they say yes.
static void start_link(struct rr_game *game, uint8_t after_prompt)
{
    EM_ASM({
        const platform = window["rrolfPlatform"];
        if (!platform)
            return;
        const put = function(string) {
            const bytes = new TextEncoder().encode(string);
            const pointer = _malloc(bytes.length + 1);
            HEAPU8.set(bytes, pointer);
            HEAPU8[pointer + bytes.length] = 0;
            return pointer;
        };
        const getToken = $1 ? platform["linkTokenAfterPrompt"] : platform["linkToken"];
        getToken().then(function(cgToken) {
            if (!cgToken)
                return;
            _rr_crazygames_on_link(put(cgToken), $0);
        });
    }, game, after_prompt);
}
#endif

void rr_crazygames_link(void *game)
{
#ifdef __EMSCRIPTEN__
    start_link(game, 0);
#endif
}

void rr_platform_copy_invite_link(char const *squad_code)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const platform = window["rrolfPlatform"];
        if (platform)
            platform["copy"](platform["inviteLink"](UTF8ToString($0)));
    }, squad_code);
#endif
}

void rr_crazygames_login(void)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const platform = window["rrolfPlatform"];
        if (platform)
            platform["login"]();
    });
#endif
}

void rr_crazygames_on_link(char *cg_token, void *game)
{
    // the game server checks it with the master server, and the answer
    // reconnects with the (possibly other) account
    rr_auth_request(game, rr_account_request_crazygames, cg_token);
    free(cg_token);
}

#ifdef __EMSCRIPTEN__
// Tells the SDK once that the game has loaded, which is when the first server
// answered.
static void tick_loading(struct rr_game *game)
{
    static uint8_t stopped;
    if (stopped || !game->socket_ready)
        return;
    EM_ASM({ window["rrolfPlatform"]["loadingStop"](); });
    stopped = 1;
}

// Gameplay is the time spent in the arena alive. Menus and the death screen
// are breaks. The SDK refuses calls that come too quickly after each other, so
// a change is only forgotten once it went through.
static void tick_gameplay(struct rr_game *game)
{
    // nothing to stop before the first start
    static int8_t reported = 0;
    uint8_t playing = game->simulation_ready && !game->flower_dead;
    if (playing == reported)
        return;
    if (EM_ASM_INT({ return window["rrolfPlatform"]["gameplay"]($0); },
                   playing))
        reported = playing;
}

// Follows the CrazyGames login: the name shown on the title screen, and the
// nickname for players who have not picked one.
static void tick_user(struct rr_game *game)
{
    static uint32_t seen = 0xffffffff;
    uint32_t version =
        EM_ASM_INT({ return window["rrolfPlatform"]["userVersion"]; });
    if (version == seen)
        return;
    seen = version;
    int32_t length = EM_ASM_INT({
        const user = window["rrolfPlatform"]["user"];
        if (!user)
            return -1;
        const bytes = new TextEncoder().encode(user["username"]);
        const used = Math.min(bytes.length, $1 - 1);
        HEAPU8.set(bytes.subarray(0, used), $0);
        HEAPU8[$0 + used] = 0;
        return used;
    }, game->cg_username, sizeof game->cg_username);
    game->cg_logged_in = length >= 0;
    if (!game->cg_logged_in)
    {
        game->cg_username[0] = 0;
        game->cg_label[0] = 0;
        return;
    }
    snprintf(game->cg_label, sizeof game->cg_label, "CrazyGames: %s",
             game->cg_username);
    if (game->cache.nickname[0] == 0)
    {
        snprintf(game->cache.nickname, sizeof game->cache.nickname, "%s",
                 game->cg_username);
        rr_dom_set_text("_0x4346", game->cache.nickname);
    }
}

// A player who just logged in to CrazyGames and has not linked their account
// is asked, with CrazyGames' own question, whether to link it now. Never
// without asking.
static void tick_login_offer(struct rr_game *game)
{
    if (!game->account_ready)
        return;
    if (!EM_ASM_INT({ return window["rrolfPlatform"]["takeLoginEvent"](); }))
        return;
    if (!game->cg_linked)
        start_link(game, 1);
}

// Shows the player's squad to CrazyGames as a room, so friends can join it
// from the CrazyGames friends list. The room id is the squad code, which
// already starts with the name of the server it lives on (hc-, pvp-).
static void tick_room(struct rr_game *game, float delta)
{
    static char sent_code[16];
    static int8_t sent_joinable = -1;
    static float cooldown;
    cooldown -= delta;
    if (cooldown > 0)
        return;
    uint8_t in_room = game->socket_ready && game->joined_squad &&
                      game->squad.squad_code[0] != 0 &&
                      game->squad.squad_code[0] != '(';
    if (!in_room)
    {
        if (sent_code[0] == 0)
            return;
        cooldown = 0.5f;
        if (EM_ASM_INT({ return window["rrolfPlatform"]["leftRoom"](); }))
        {
            sent_code[0] = 0;
            sent_joinable = -1;
        }
        return;
    }
    uint8_t members = 0;
    for (uint32_t i = 0; i < RR_SQUAD_MEMBER_COUNT; ++i)
        members += game->squad.squad_members[i].in_use != 0;
    int8_t joinable = members < RR_SQUAD_MEMBER_COUNT;
    if (joinable == sent_joinable && strcmp(sent_code, game->squad.squad_code) == 0)
        return;
    cooldown = 0.5f;
    if (EM_ASM_INT({
            return window["rrolfPlatform"]["room"](UTF8ToString($0), $1);
        }, game->squad.squad_code, joinable))
    {
        snprintf(sent_code, sizeof sent_code, "%s", game->squad.squad_code);
        sent_joinable = joinable;
    }
}

// the squad code the player was last sent to by an invite, "" if none
static char invited_code[16];

// A friend invited the player, at startup (invite link) or while playing.
// Joins that squad, which also switches to the right server.
static void tick_join(struct rr_game *game)
{
    if (!game->account_ready || game->socket_pending)
        return;
    if (!EM_ASM_INT({ return +!!window["rrolfPlatform"]["joinCode"]; }))
        return;
    char code[16];
    EM_ASM({
        const bytes = new TextEncoder().encode(
            window["rrolfPlatform"]["takeJoinCode"]());
        HEAPU8.set(bytes, $0);
        HEAPU8[$0 + bytes.length] = 0;
    }, code);
    if (strlen(code) == 0 || strcmp(code, game->squad.squad_code) == 0)
        return;
    snprintf(invited_code, sizeof invited_code, "%s", code);
    rr_game_join_squad_by_code(game, code);
}

static void send_squad_packet(struct rr_game *game, uint8_t header,
                              int join_type)
{
    struct proto_bug encoder;
    proto_bug_init(&encoder, RR_OUTGOING_PACKET);
    proto_bug_write_uint8(&encoder, game->socket.quick_verification, "qv");
    proto_bug_write_uint8(&encoder, header, "header");
    if (join_type >= 0)
        proto_bug_write_uint8(&encoder, join_type, "join type");
    rr_websocket_send(&game->socket, encoder.current - encoder.start);
}

// "Instant multiplayer": CrazyGames wants the player in a joinable room and
// playing without touching the menu. Joins the invited squad if there is one,
// otherwise makes a new private squad (which friends can then join), and then
// presses ready. Runs once per visit.
static void tick_instant(struct rr_game *game, float delta)
{
    static uint8_t state; // 0 not started, 1 waiting for the squad, 2 done
    static char baseline[16];
    static float waited;
    if (state == 2)
        return;
    if (state == 0)
    {
        if (!game->account_ready || game->socket_pending)
            return;
        if (!EM_ASM_INT({ return +!!window["rrolfPlatform"]["instant"]; }))
        {
            state = 2;
            return;
        }
        // the code of the last squad, so a new one can be told apart
        snprintf(baseline, sizeof baseline, "%s", game->squad.squad_code);
        if (invited_code[0] == 0)
            send_squad_packet(game, rr_serverbound_squad_join, 2);
        state = 1;
        waited = 0;
        return;
    }
    waited += delta;
    // the server ignores two squad actions within 0.4s
    if (waited < 0.6f)
        return;
    uint8_t arrived =
        game->joined_squad &&
        (invited_code[0] ? strcmp(game->squad.squad_code, invited_code) == 0
                         : strcmp(game->squad.squad_code, baseline) != 0);
    if (!arrived && waited < 4.0f)
        return;
    send_squad_packet(game, rr_serverbound_squad_ready, -1);
    state = 2;
}
#endif

void rr_platform_tick(struct rr_game *game, float delta)
{
#ifdef __EMSCRIPTEN__
    // an invite link works on a plain web page as well
    tick_join(game);
    if (!rr_platform_on_crazygames())
        return;
    // hiding the interface hides things CrazyGames' QA needs to see
    game->cache.hide_ui = 0;
    tick_loading(game);
    tick_gameplay(game);
    tick_user(game);
    tick_login_offer(game);
    tick_instant(game, delta);
    tick_room(game, delta);
#endif
}
