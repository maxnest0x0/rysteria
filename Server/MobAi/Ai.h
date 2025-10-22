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

#pragma once

#include <stdlib.h>

#include <Shared/Entity.h>
#include <Shared/Vector.h>

struct rr_simulation;
struct rr_component_ai;

uint8_t has_new_target(struct rr_component_ai *, struct rr_simulation *);
uint8_t ai_is_passive(struct rr_component_ai *);
uint8_t should_aggro(struct rr_simulation *, struct rr_component_ai *);
struct rr_vector predict(struct rr_vector, struct rr_vector, float);

void tick_idle(EntityIdx, struct rr_simulation *);
void tick_idle_move_default(EntityIdx, struct rr_simulation *);
void tick_idle_move_sinusoid(EntityIdx, struct rr_simulation *, float);
uint8_t tick_summon_return_to_owner(EntityIdx, struct rr_simulation *);
void tick_return_to_higher_zone(EntityIdx, struct rr_simulation *);

void tick_ai_default(EntityIdx, struct rr_simulation *, float);
void tick_ai_triceratops(EntityIdx, struct rr_simulation *);
void tick_ai_trex(EntityIdx, struct rr_simulation *);
void tick_ai_pteranodon(EntityIdx, struct rr_simulation *);
void tick_ai_pachycephalosaurus(EntityIdx, struct rr_simulation *);
void tick_ai_ornithomimus(EntityIdx, struct rr_simulation *);
void tick_ai_ankylosaurus(EntityIdx, struct rr_simulation *);
void tick_ai_meteor(EntityIdx, struct rr_simulation *);
void tick_ai_quetzalcoaltus(EntityIdx, struct rr_simulation *);
