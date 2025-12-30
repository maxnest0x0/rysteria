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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Socket.h>
#include <Shared/Api.h>
#include <Shared/Bitset.h>
#include <Shared/MagicNumber.h>
#include <Shared/Rivet.h>

#ifndef EMSCRIPTEN
#include <glfw/GLFW3.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

static void *rr_create_game_thread(void *arg)
{
    struct rr_game *game = arg;

    while (1)
    {
        struct timeval start;
        struct timeval end;
        gettimeofday(&start, NULL);
        rr_game_tick(game, 0.016f);
        gettimeofday(&end, NULL);
        int64_t elapsed_time = (end.tv_sec - start.tv_sec) * 1000000 +
                               (end.tv_usec - start.tv_usec);
        if (elapsed_time > 1000)
            printf("tick took %ld microseconds\n", elapsed_time);
        int64_t to_sleep = 16666 - elapsed_time;
        if (to_sleep > 0)
            usleep(to_sleep);
    }

    return 0;
}

#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <Client/DOM.h>

#define troll_skids                                                            \
    if (!trusted)                                                              \
    {                                                                          \
        rr_page_open("https://www.youtube.com/watch?v=dQw4w9WgXcQ", 1);        \
        return;                                                                \
    }

void rr_key_event(struct rr_game *this, uint8_t type, uint32_t which,
                  uint32_t key, uint8_t repeat, uint8_t trusted)
{
    troll_skids
    if (repeat)
        return;
    if (type == 1)
    {
        rr_bitset_set(this->input_data->keys_pressed, which);
        rr_bitset_set(this->input_data->keys_pressed_this_tick, which);
        if (this->input_data->keycodes_length < 16 && key != 0)
            this->input_data->keycodes_pressed_this_tick
                [this->input_data->keycodes_length++] = key;
    }
    else
    {
        rr_bitset_unset(this->input_data->keys_pressed, which);
        rr_bitset_unset(this->input_data->keys_released_this_tick, which);
    }
}

void rr_mouse_event(struct rr_game *this, float x, float y, uint8_t state,
                    uint8_t button, uint8_t trusted)
{
    troll_skids
    this->input_data->mouse_x = x;
    this->input_data->mouse_y = y;
    this->input_data->mouse_state = state;
    if (state == 1) // press down
    {
        this->input_data->mouse_buttons |= (1 << button);
        this->input_data->mouse_buttons_down_this_tick |= (1 << button);
    }
    else if (state == 0)
    {
        this->input_data->mouse_buttons &= ~(1 << button);
        this->input_data->mouse_buttons_up_this_tick |= (1 << button);
    }
}

void rr_touch_event(struct rr_game *this, float x, float y, uint8_t state,
                    uint8_t identifier, uint8_t trusted)
{
    troll_skids
    struct rr_input_touch *touch = &this->input_data->touches[identifier % 16];
    touch->touch_x = x;
    touch->touch_y = y;
    touch->identifier = identifier;
    touch->active = state;
}

void rr_wheel_event(struct rr_game *this, float delta, uint8_t trusted)
{
    troll_skids
    this->input_data->scroll_delta = delta;
}

void rr_paste_event(struct rr_game *this, char *buf, uint8_t trusted)
{
    if (!trusted)
        free(buf);
    troll_skids
    if (this->input_data->clipboard != NULL)
        free(this->input_data->clipboard);
    this->input_data->clipboard = buf;
}

void rr_context_event(struct rr_renderer *renderer, uint8_t type,
                      uint8_t trusted)
{
    troll_skids
    if (type == 1 && renderer->on_context_restore != NULL)
        renderer->on_context_restore(renderer->on_context_restore_captures);
}

static void release_key(uint64_t i, void *captures)
{
    struct rr_game *this = captures;
    rr_bitset_set(this->input_data->keys_released_this_tick, i);
}

void rr_focus_event(struct rr_game *this, uint8_t type, uint8_t trusted)
{
    troll_skids
    if (type == 1)
    {
        this->input_data->mouse_buttons_up_this_tick |=
            this->input_data->mouse_buttons;
        this->input_data->mouse_buttons = 0;
        rr_bitset_for_each_bit(
            this->input_data->keys_pressed,
            this->input_data->keys_pressed + RR_BITSET_ROUND(256),
            this, release_key);
        memset(this->input_data->keys_pressed, 0, RR_BITSET_ROUND(256));
    }
}
#undef troll_skids
#else
#endif

void rr_main_loop(struct rr_game *this)
{
    printf("client on version %llu\n", RR_SECRET8 ^ 255);
    srand(time(0));
#ifdef EMSCRIPTEN
    EM_ASM(
        {
            Module.canvas = document.createElement("canvas");
            Module.canvas.id = "canvas";
            Module.canvas.oncontextmenu = function() { return false; };
            window.onbeforeunload = function(e) { return "Are you sure?"; };
            document.body.appendChild(Module.canvas);
            Module.ctxs = [Module.canvas.getContext('2d')];
            Module.availableCtxs =
                new Array(256).fill(0).map(function(_, i) { return i; });
            window.addEventListener("keydown", function(e)
            {
                _rr_key_event(
                    $0, 1, e.which,
                    e.key ? (!e.ctrlKey && !e.metaKey && e.key.length == 1) *
                                e.key.charCodeAt()
                          : 0, +e.repeat, +e.isTrusted);
            });
            window.addEventListener("keyup", function(e)
            {
                _rr_key_event(
                    $0, 0, e.which,
                    e.key ? (!e.ctrlKey && !e.metaKey && e.key.length == 1) *
                                e.key.charCodeAt()
                          : 0, +e.repeat, +e.isTrusted);
            });
            window.addEventListener(
                "mousedown", async function(e) {
                    const clientX = e.clientX;
                    const clientY = e.clientY;
                    const button = e.button;
                    _rr_mouse_event($0, clientX * devicePixelRatio,
                                    clientY * devicePixelRatio, 1, +!!button,
                                    +e.isTrusted);
                });
            window.addEventListener(
                "mousemove", async function(e) {
                    const clientX = e.clientX;
                    const clientY = e.clientY;
                    const button = e.button;
                    _rr_mouse_event($0, clientX * devicePixelRatio,
                                    clientY * devicePixelRatio, 2, +!!button,
                                    +e.isTrusted);
                });
            window.addEventListener(
                "mouseup", async function(e) {
                    const clientX = e.clientX;
                    const clientY = e.clientY;
                    const button = e.button;
                    _rr_mouse_event($0, clientX * devicePixelRatio,
                                    clientY * devicePixelRatio, 0, +!!button,
                                    +e.isTrusted);
                });
            window.addEventListener(
                "touchstart",
                function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    if (!e.changedTouches.length)
                        return;
                    const touch = e.changedTouches[0];
                    _rr_mouse_event($0, touch.clientX * devicePixelRatio,
                                    touch.clientY * devicePixelRatio, 1, 0,
                                    +e.isTrusted);
                    for (const t of e.changedTouches)
                        _rr_touch_event($0, t.clientX * devicePixelRatio,
                                        t.clientY * devicePixelRatio, 1,
                                        t.identifier, +e.isTrusted);
                },
                {passive : false});
            window.addEventListener(
                "touchmove",
                function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    if (!e.changedTouches.length)
                        return;
                    const touch = e.changedTouches[0];
                    _rr_mouse_event($0, touch.clientX * devicePixelRatio,
                                    touch.clientY * devicePixelRatio, 2, 0,
                                    +e.isTrusted);
                    for (const t of e.changedTouches)
                        _rr_touch_event($0, t.clientX * devicePixelRatio,
                                        t.clientY * devicePixelRatio, 1,
                                        t.identifier, +e.isTrusted);
                },
                {passive : false});
            window.addEventListener(
                "touchend",
                function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    if (!e.changedTouches.length)
                        return;
                    const touch = e.changedTouches[0];
                    _rr_mouse_event($0, touch.clientX * devicePixelRatio,
                                    touch.clientY * devicePixelRatio, 0, 0,
                                    +e.isTrusted);
                    for (const t of e.changedTouches)
                        _rr_touch_event($0, t.clientX * devicePixelRatio,
                                        t.clientY * devicePixelRatio, 0,
                                        t.identifier, +e.isTrusted);
                },
                {passive : false});
            window.addEventListener(
                "wheel",
                async function(e) {
                    _rr_wheel_event($0, e.deltaY, +e.isTrusted);
                });
            document.body.addEventListener(
                "paste", async function(e) {
                    const buf = new TextEncoder().encode(
                        e.clipboardData.getData("text/plain"));
                    const $a = _malloc(buf.length + 1);
                    HEAPU8.set(buf, $a);
                    HEAPU8[$a + buf.length] = 0;
                    _rr_paste_event($0, $a, +e.isTrusted);
                });
            window.addEventListener("blur", function(e) {
                _rr_focus_event($0, 1, +e.isTrusted);
            });
            Module.addCtx = function(renderer)
            {
                if (Module.availableCtxs.length)
                {
                    const index = Module.availableCtxs.shift();
                    if (index == 0)
                        return 0; // used for the main ctx, because that has
                                  // special behavior
                    const ocanvas = new OffscreenCanvas(1, 1);
                    ocanvas.addEventListener("contextrestored", function(e) {
                        _rr_context_event(renderer, 1, +e.isTrusted);
                    });
                    Module.ctxs[index] = ocanvas.getContext('2d');
                    return index;
                }
                throw new Error(
                    'Out of Contexts: Can be fixed by allowing more contexts');
                return -1;
            };
            Module.removeCtx = function(index)
            {
                if (index == 0)
                    throw new Error('Tried to delete the main context');
                Module.ctxs[index] = null;
                Module.availableCtxs.push(index);
            };

            function loop(time)
            {
                const delta = Math.min(0.5, (time - Module.start) / 1000);
                Module.start = time;
                Module.canvas.width =
                    document.documentElement.clientWidth * devicePixelRatio;
                Module.canvas.height =
                    document.documentElement.clientHeight * devicePixelRatio;
                _rr_renderer_main_loop($0, delta, Module.canvas.width,
                                       Module.canvas.height, devicePixelRatio);
                requestAnimationFrame(loop);
            };
            Module.start = 0;
            requestAnimationFrame(loop);
        },
        this, &this->input_data->mouse_x, &this->input_data->mouse_state);
#endif
}

void rr_renderer_main_loop(struct rr_game *this, float delta, float width,
                           float height, float device_pixel_ratio)
{
    float a = height / 1080;
    float b = width / 1920;

    float scale = (this->renderer->scale = b < a ? a : b) * device_pixel_ratio;
    this->renderer->width = width;
    this->renderer->height = height;
    this->window->width = this->window->abs_width = width / this->renderer->scale;
    this->window->height = this->window->abs_height = height / this->renderer->scale;
    rr_game_tick(this, delta);
    this->input_data->scroll_delta = 0;
}

int main()
{
    static struct rr_game game;
    static struct rr_renderer renderer;
    static struct rr_input_data input_data;
    struct rr_simulation *simulation = malloc(sizeof *simulation);
    struct rr_simulation *deletion_simulation =
        malloc(sizeof *deletion_simulation);
    rr_main_loop(&game);

    rr_renderer_init(&renderer);
    rr_game_init(&game);
    rr_input_data_init(&input_data);
    rr_simulation_init(simulation);
    rr_simulation_init(deletion_simulation);

    game.renderer = &renderer;
    game.input_data = &input_data;
    game.simulation = simulation;
    game.deletion_simulation = deletion_simulation;
    rr_game_tick(&game, 1);

#ifndef EMSCRIPTEN
    // glfwSetErrorCallback([](int error, char const *description)
    //                      { std::cerr << "code " << error << ' ' <<
    //                      description << '\n'; });

    glfwInit();
    GLFWwindow *window =
        glfwCreateWindow(800, 800, "rrolf native client", NULL, NULL);

    // pthread_t socket_tid;

    // if (pthread_create(&socket_tid, NULL, rr_create_game_thread, &game) != 0)
    // {
    //     fputs("Failed to create game thread\n", stderr);
    //     return 1;
    // }

    // rr_game_connect_socket(&game);
    rr_create_game_thread(&game);
#endif

    return 0;
}
