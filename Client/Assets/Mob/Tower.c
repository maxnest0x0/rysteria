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

#include <Client/Assets/Render.h>

#include <Client/Renderer/Renderer.h>

void rr_tower_draw(struct rr_renderer *renderer)
{
    rr_renderer_set_fill(renderer, 0xff404040);
    rr_renderer_set_stroke(renderer, 0xff303030);
    rr_renderer_set_line_width(renderer, 3.0f);
    rr_renderer_begin_path(renderer);
    rr_renderer_set_line_cap(renderer, 1);
    rr_renderer_set_line_join(renderer, 1);
    rr_renderer_set_line_width(renderer, 3);
    rr_renderer_move_to(renderer, 0.00, -140.0);
    rr_renderer_line_to(renderer, 140.0, 0.00);
    rr_renderer_line_to(renderer, 0.00, 140.0);
    rr_renderer_line_to(renderer, -140.0, 0.00);
    rr_renderer_line_to(renderer, 0.00, -140.0);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
    
    rr_renderer_set_fill(renderer, 0xff404040);
    rr_renderer_set_stroke(renderer, 0xff303030);
    rr_renderer_set_line_width(renderer, 3.0f);
    rr_renderer_begin_path(renderer);
    rr_renderer_set_line_cap(renderer, 1);
    rr_renderer_set_line_join(renderer, 1);
    rr_renderer_set_line_width(renderer, 3);
    rr_renderer_move_to(renderer, -85.00, -85.0);
    rr_renderer_line_to(renderer, -85.0, 85.00);
    rr_renderer_line_to(renderer, 85.00, 85.0);
    rr_renderer_line_to(renderer, 85.0, -85.00);
    rr_renderer_line_to(renderer, -85.00, -85.0);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);

    rr_renderer_begin_path(renderer);
    rr_renderer_set_stroke(renderer, 0xff303030);
    rr_renderer_set_fill(renderer, 0xff404040);
    rr_renderer_set_line_width(renderer, 3.0f);
    rr_renderer_move_to(renderer, 0.00, -110.0);
    rr_renderer_line_to(renderer, 110.0, 0.00);
    rr_renderer_line_to(renderer, 0.00, 110.0);
    rr_renderer_line_to(renderer, -110.0, 0.00);
    rr_renderer_line_to(renderer, 0.00, -110.0);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);

    rr_renderer_set_stroke(renderer, 0xff870BF4);
    rr_renderer_set_fill(renderer, 0xff9527F5);
    rr_renderer_set_line_width(renderer, 3.0f);
    rr_renderer_begin_path(renderer);
    rr_renderer_arc(renderer, 0.0f, 0.0f, 60.0f);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);

    rr_renderer_begin_path(renderer);
    rr_renderer_set_stroke(renderer, 0xff870BF4);
    rr_renderer_set_line_width(renderer, 10.0f);
    rr_renderer_move_to(renderer, -42.0, -42.0);
    rr_renderer_line_to(renderer, 42.0, 42.0);
    rr_renderer_move_to(renderer, -42.0, 42.0);
    rr_renderer_line_to(renderer, 42.0, -42.0);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
    
    rr_renderer_begin_path(renderer);
    rr_renderer_set_stroke(renderer, 0xff9527F5);
    rr_renderer_set_line_width(renderer, 4.0f);
    rr_renderer_move_to(renderer, -42.0, -42.0);
    rr_renderer_line_to(renderer, 42.0, 42.0);
    rr_renderer_move_to(renderer, -42.0, 42.0);
    rr_renderer_line_to(renderer, 42.0, -42.0);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
}