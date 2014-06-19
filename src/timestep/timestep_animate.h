/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.
 
 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

#ifndef TIMESTEP_ANIMATE_H
#define TIMESTEP_ANIMATE_H

#include "core/timestep/timestep.h"

#include "core/object_pool.h"
#include "core/list.h"

 //get a new animation frame. Don't free this object, call release
anim_frame *anim_frame_get();
//release an animation frame.  You should never call free yourself
void anim_frame_release(anim_frame *frame);

style_prop *anim_frame_add_style_prop(anim_frame *frame);

//construct a new animation.  Don't free this object, call release
view_animation *view_animation_init(struct timestep_view_t *view);
//release an animation.  Don't ever call free on one.
void view_animation_release(view_animation *anim);

void view_animation_pause(view_animation *anim);
void view_animation_resume(view_animation *anim);

//finish running the animation this tick
void view_animation_commit(view_animation *anim);
//reset the view animation to its default state.  Clears frames
void view_animation_clear(view_animation *anim);
//Given a frame, a duration, and a transition start running that frame on the animation now
void view_animation_now(view_animation *anim, anim_frame *frame, unsigned int duration, unsigned int transition);
//chain a new frame to run after the current one
void view_animation_then(view_animation *anim, anim_frame *frame, unsigned int duration, unsigned int transition);
//put a 'wait' duration milliseconds  on the queue
void view_animation_wait(view_animation *anim, unsigned int duration);

CEXPORT void view_animation_tick_animations(long dt);

// Shutdown subsystem
CEXPORT void view_animation_shutdown();

#endif
