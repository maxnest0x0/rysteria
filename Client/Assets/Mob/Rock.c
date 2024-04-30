
#include <Client/Assets/Render.h>

#include <Client/Renderer/Renderer.h>

void rr_rock_draw(struct rr_renderer *renderer)
{
    rr_renderer_set_fill(renderer, 0xff666666);
    rr_renderer_begin_path(renderer);
    rr_renderer_fill(renderer);
}
