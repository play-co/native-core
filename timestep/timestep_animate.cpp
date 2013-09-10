/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the Game Closure SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "core/timestep/timestep_animate.h"
#include "core/timestep/timestep_view.h"
#include "js/js.h"
#include "js/js_animate.h"
#include "core/log.h"

/*
 * make some object pools for our different objects
 */
static object_pool *view_animation_pool = OBJECT_POOL_INIT(view_animation, 64);
static object_pool *style_prop_pool = OBJECT_POOL_INIT(style_prop, 64);
static object_pool *frame_pool = OBJECT_POOL_INIT(anim_frame, 32);

// Every animation goes on this global list so we can tick them all
static view_animation *global_head = NULL;

static void init_frame(anim_frame *frame, struct timestep_view_t *v);
static void apply_frame(anim_frame *frame, struct timestep_view_t *view, double tt);
static void view_animation_tick(view_animation *anim, int dt);

/*
 * if we are currently in the middle of a tick, this is true and therefore
 * we shouldn't tick any animation that was added until the next tick
 *
 */
static view_animation **global_list_buffer = NULL;
static unsigned int global_list_buffer_size = 0;

static int global_frame_uid = 0;
anim_frame *anim_frame_get() {
	anim_frame *frame = OBJECT_POOL_GET(anim_frame, frame_pool);
	frame->prop_head = NULL;
	frame->resolved = false;
	frame->id = global_frame_uid++;

	js_object_wrapper_init(&frame->cb);

	return frame;
}

void anim_frame_release(anim_frame *frame) {
	style_prop **prop_head = &frame->prop_head;
	style_prop *curr;
	while (*prop_head) {
		curr = *prop_head;
		LIST_REMOVE(prop_head, curr);
		OBJECT_POOL_RELEASE(curr);
	}

	js_object_wrapper_delete(&frame->cb);

	OBJECT_POOL_RELEASE(frame);
}

style_prop *anim_frame_add_style_prop(anim_frame *frame) {
	style_prop *prop = OBJECT_POOL_GET(style_prop, style_prop_pool);
	prop->is_delta = false;
	LIST_ADD(&frame->prop_head, prop);
	return prop;
}

view_animation *view_animation_init(timestep_view *view) {
	LOGFN("view_animation_init");
	view_animation *anim = OBJECT_POOL_GET(view_animation, view_animation_pool);
	anim->frame_head = NULL;
	anim->view = view;
	anim->elapsed = 0;
	anim->is_scheduled = false;
	anim->is_paused = false;

	timestep_view_add_animation(view, anim);

	js_object_wrapper_init(&anim->js_group);

	LOGFN("end view_animation_init");
	return anim;
}

void view_animation_release(view_animation *anim) {
	LOGFN("view_animation_release");

	view_animation_clear(anim);
	if (anim->is_scheduled) {
		anim->is_scheduled = false;
		LIST_REMOVE(&global_head, anim);
	}

	js_object_wrapper_delete(&anim->js_group);

	OBJECT_POOL_RELEASE(anim);

	LOGFN("end view_animation_release");
}

void view_animation_schedule(view_animation *anim) {
	if (!anim->is_scheduled) {
		anim->is_scheduled = true;
		LIST_ADD(&global_head, anim);
	}
}

void view_animation_unschedule(view_animation *anim) {
	if (anim->is_scheduled) {
		anim->is_scheduled = false;
		LIST_REMOVE(&global_head, anim);
	}
}

void view_animation_pause(view_animation *anim) {
	if (!anim->is_paused) {
		anim->is_paused = true;
		view_animation_unschedule(anim);
	}
}

void view_animation_resume(view_animation *anim) {
	if (anim->is_paused) {
		anim->is_paused = false;
		view_animation_schedule(anim);
	}
}

void view_animation_clear(view_animation *anim) {
	LOGFN("view_animation_clear");

	anim_frame **head = &anim->frame_head;
	anim_frame *curr;

	while (*head) {
		curr = *head;
		LIST_REMOVE(head, curr);
		anim_frame_release(curr);
	}

	view_animation_unschedule(anim);
	anim->elapsed = 0;
	LOGFN("end view_animation_clear");
}

void view_animation_commit(view_animation *anim) {
	LOGFN("view_animation_commit");
	unsigned int elapsed = 0;

	anim_frame **head = &anim->frame_head;
	anim_frame *curr = *head;
	while (curr) {
		elapsed += curr->duration;
		LIST_ITERATE(head, curr);
	}

	view_animation_tick(anim, elapsed);

	LOGFN("end view_animation_commit");
}

void view_animation_wait(view_animation *anim, unsigned int duration) {
	LOGFN("view_animation_wait");
	anim_frame *frame = anim_frame_get();
	//anim->is_running = true; TODO
	frame->type = WAIT_FRAME;
	view_animation_then(anim, frame, duration, (unsigned int)NULL);

	LOGFN("end view_animation_wait");
}

void view_animation_now(view_animation *anim, anim_frame *frame, unsigned int duration, unsigned int transition) {
	LOGFN("view_animation_now");
	if (transition == NO_TRANSITION) {
		transition = anim->frame_head ? EASE_OUT : EASE_IN_OUT;
	}

	view_animation_clear(anim);
	view_animation_then(anim, frame, duration, transition);
	LOGFN("end view_animation_now");
}

void view_animation_then(view_animation *anim, anim_frame *frame, unsigned int duration, unsigned int transition) {
	LOGFN("view_animation_then");

	view_animation_schedule(anim);

	LIST_ADD(&anim->frame_head, frame);
	frame->duration = duration;

	if (transition == NO_TRANSITION) {
		transition = EASE_IN_OUT;
	}

	frame->transition = transition;
	//anim->is_running = true; TODO what should this do?

	LOGFN("end view_animation_then");
}

CEXPORT void view_animation_tick_animations(int dt) {
	LOGFN("view_animation_tick_animations");
	// lock the animation queue - kind of hacky, but should handle
	// *most* situations where we modify the queue while ticking
	// (currently handles the case of adding new animations during
	// tick...)

	view_animation *curr;
	view_animation *next;

	// copy the list into the global buffer
	unsigned int count = 0;
	next = global_head;
	while (next) {
		curr = next;
		LIST_ITERATE(&global_head, next);

		if (count + 1 > global_list_buffer_size) {
			if (!global_list_buffer_size) {
				global_list_buffer_size = 32;
				global_list_buffer = (view_animation **) malloc(sizeof(view_animation *) * global_list_buffer_size);
			} else {
				global_list_buffer_size *= 2;
				global_list_buffer = (view_animation **) realloc(global_list_buffer, sizeof(view_animation *) * global_list_buffer_size);
			}
		}

		global_list_buffer[count] = curr;
		++count;
	}

	unsigned int i;
	for (i = 0; i < count; ++i) {
		view_animation_tick(global_list_buffer[i], dt);
	}

	//LOG("==== done ticking animations ====");
	//curr = global_head;
	//while (curr) {
	//	LOG(">> view %i", curr->view->uid);
	//	LIST_ITERATE(&global_head, curr);
	//}
	//LOG("==================================");

	LOGFN("end view_animation_tick_animations");
}

// exports.linear = function (n) { return n; }
#define FN_LINEAR(n) n
// exports.easeIn = function (n) { return n * n; }
#define FN_EASE_IN(n) (n * n)
// exports.easeInOut = function (n) { return (n *= 2) < 1 ? 0.5 * n * n * n : 0.5 * ((n -= 2) * n * n + 2); }
#define FN_EASE_IN_OUT(n) ((n *= 2) < 1 ? 0.5 * n * n * n : 0.5 * ((n - 2) * (n - 2) * (n - 2) + 2))
// exports.easeOut = function(n) { return n * (2 - n); }
#define FN_EASE_OUT(n) (n * (2 - n))
// TODO
#define FN_BOUNCE(n) n

static double apply_transition(anim_frame *frame, double t) {
	switch (frame->transition) {
		case LINEAR: return FN_LINEAR(t);
		case EASE_IN: return FN_EASE_IN(t);
		case EASE_OUT: return FN_EASE_OUT(t);
		case BOUNCE: return FN_BOUNCE(t);
		case EASE_IN_OUT:
		default: return FN_EASE_IN_OUT(t);
	}
}

// iterate over all the style properties in a frame and set
// the initial value from the current view style
static void init_frame(anim_frame *frame, timestep_view *v) {
	LOGFN("init_frame");

	style_prop **head = &frame->prop_head;
	style_prop *curr = *head;
	while (curr) {
		// copy the initial value from the view
		#define SET_INITIAL_PROP(name, prop) case name: curr->initial = v->prop; break;
		switch (curr->name) {
			SET_INITIAL_PROP(X, x)
			SET_INITIAL_PROP(Y, y)
			SET_INITIAL_PROP(WIDTH, width)
			SET_INITIAL_PROP(HEIGHT, height)
			SET_INITIAL_PROP(R, r)
			SET_INITIAL_PROP(ANCHOR_X, anchor_x)
			SET_INITIAL_PROP(ANCHOR_Y, anchor_y)
			SET_INITIAL_PROP(OPACITY, opacity)
			SET_INITIAL_PROP(SCALE, scale)
			SET_INITIAL_PROP(SCALE_X, scale_x)
			SET_INITIAL_PROP(SCALE_Y, scale_y)
		}

		// if the prop is not a delta, we must compute the delta
		// WARNING: target/delta is a union, so target is not preserved
		if (curr->is_delta) {
			curr->delta = curr->target;
		} else {
			curr->delta = curr->target - curr->initial;
		}

		LIST_ITERATE(head, curr);
	}

	LOGFN("end init_frame");
}

static void apply_frame(anim_frame *frame, timestep_view *view, double tt) {
	LOGFN("apply_frame");
	style_prop **head = &frame->prop_head;
	style_prop *curr = *head;
	while (curr) {
		// copy the initial value from the view
		 //#define UPDATE_PROP(name, prop) case name: LOG("view prop is %i %f time %f delta %f initial %f", name, view->prop, tt, curr->delta, curr->initial); view->prop = curr->delta * tt + curr->initial; break;
		#define UPDATE_PROP(name, prop) case name: view->prop = curr->delta * tt + curr->initial; break;
		switch (curr->name) {
			UPDATE_PROP(X, x)
			UPDATE_PROP(Y, y)
			UPDATE_PROP(WIDTH, width)
			UPDATE_PROP(HEIGHT, height)
			UPDATE_PROP(R, r)
			UPDATE_PROP(ANCHOR_X, anchor_x)
			UPDATE_PROP(ANCHOR_Y, anchor_y)
			UPDATE_PROP(OPACITY, opacity)
			UPDATE_PROP(SCALE, scale)
			UPDATE_PROP(SCALE_X, scale_x)
			UPDATE_PROP(SCALE_Y, scale_y)
		}

		LIST_ITERATE(head, curr);
	}

	LOGFN("end apply_frame");
}

static void view_animation_tick(view_animation *anim, int dt) {
	LOGFN("view_animation_tick");

	if (!anim->is_scheduled) {
		return;
	}

	timestep_view *view = anim->view;
	if (!view) {
		LOG("WARNING: Animation tick terminated early because view died");
		view_animation_unschedule(anim);
		def_animate_finish(anim->js_anim);
		return;
	}

	anim_frame **frame_head = &anim->frame_head;
	anim_frame *frame = anim->frame_head;
	anim->elapsed += dt;

	//LOG("/ticking anim for %i it's a %i", view->uid, frame->type);
	while (frame) {
		unsigned int cur_frame_id = frame->id;
		bool frame_finished = anim->elapsed >= frame->duration;
		double t = frame_finished ? 1 : ((double) anim->elapsed) / frame->duration;
		double tt = frame_finished ? 1 : apply_transition(frame, t);

		if (frame_finished) {
			anim->elapsed -= frame->duration;
		}

		switch (frame->type) {
			case WAIT_FRAME:

				break;
			case STYLE_FRAME:
				//LOG("it's a style frame %f", t);
				// the first time the frame runs, grab a copy of the current
				// style so that we can tween from the original style to the target style.
				if (!frame->resolved) {
					//LOG("resolving style frame %p", frame);
					frame->resolved = true;
					init_frame(frame, view);
				}

				// iterate over all the style properties
				//LOG("applying %p", frame);
				apply_frame(frame, view, tt);
				break;
			case FUNC_FRAME:
				//LOG("calling func frame %i %i", dt);
				//WARN: If the callback calls clear then you can potentially get the
				//same frame pointer back if any animate's are called thereafter.
				def_animate_cb(view->js_view, frame->cb, tt, t);
				break;
			default:
				break;
		}
        if (isnan(view->x) || isnan(view->y) || isnan(view->width) || isnan(view->height)) {
            //LOG("animated to NaN?");
        }
		// if the frame is finished, remove it. However, if the frame head is not equal to the
		// what was the current frame's id, then this frame is different than the one used in the
		// frame type switch statement, and should be left alone.
		if (frame_finished) {

			// if we haven't modified the queue in a callback, remove the frame if it is finished
			if (frame == *frame_head && cur_frame_id == frame->id) {
				LIST_REMOVE(frame_head, frame);
				anim_frame_release(frame);
			}

			frame = *frame_head;
		}

		// if we got paused during a callback or the
		// frame is not finished yet, don't continue
		if (!frame_finished || anim->is_paused) {
			return;
		}
	}

	view_animation_unschedule(anim);
	def_animate_finish(anim->js_anim);
	LOGFN("end view_animation_tick");
}

CEXPORT void view_animation_shutdown() {
	// Remove all animations
	while (global_head) {
		view_animation_release(global_head);
	}
}
