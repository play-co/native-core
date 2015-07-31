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

// Core objects of the timestep subsystem

#ifndef TIMESTEP_H
#define TIMESTEP_H

#include <float.h>

#include "core/util/detect.h"
#include "js/js.h"
#include "core/rgba.h"
#include "core/tealeaf_context.h"


struct view_animation_t;


//// View

#define UNDEFINED_DIMENSION DBL_MIN

enum view_types { DEFAULT_RENDER, IMAGE_VIEW };

typedef struct timestep_view_t {
	unsigned int uid;
	struct timestep_view_t **subviews;
	struct timestep_view_t *superview;
	unsigned int subview_array_size;
	unsigned int subview_count;
	unsigned int subview_index;

	int added_at;

  JS_OBJECT_WRAPPER js_view;
	bool has_jsrender;
	bool has_jstick;

	double x;
	double y;
	double width;
	double height;
	double r;
	double anchor_x;
	double anchor_y;
	double offset_x;
	double offset_y;
	double scale;
	double scale_x;
	double scale_y;
	double abs_scale;
	double opacity;
	bool needs_reflow;
	bool clip;
	bool visible;
	bool flip_x;
	bool flip_y;

	int composite_operation;

	struct rgba_t background_color;
	int z_index;
	bool dirty_z_index;

	struct view_animation_t **anims;
	unsigned int anim_count;
	unsigned int max_anims;

	void (*timestep_view_render)(struct timestep_view_t*, context_2d*);
	void (*timestep_view_tick)(struct timestep_view_t*, double);

	PERSISTENT_JS_OBJECT_WRAPPER map_ref;
	void *view_data;

	rgba filter_color;
	int filter_type;
} timestep_view;


//// Animation

enum frame_type {
  WAIT_FRAME,
  STYLE_FRAME,
  FUNC_FRAME
};
enum style_props {
  X,
  Y,
  WIDTH,
  HEIGHT,
  R,
  ANCHOR_X,
  ANCHOR_Y,
  OPACITY,
  SCALE,
  SCALE_X,
  SCALE_Y
};
enum transitions {
  NO_TRANSITION,
  LINEAR,
  EASE_IN,
  EASE_OUT,
  EASE_IN_OUT,
  EASE_IN_QUAD,
  EASE_OUT_QUAD,
  EASE_IN_OUT_QUAD,
  EASE_IN_CUBIC,
  EASE_OUT_CUBIC,
  EASE_IN_OUT_CUBIC,
  EASE_IN_QUART,
  EASE_OUT_QUART,
  EASE_IN_OUT_QUART,
  EASE_IN_QUINT,
  EASE_OUT_QUINT,
  EASE_IN_OUT_QUINT,
  EASE_IN_SINE,
  EASE_OUT_SINE,
  EASE_IN_OUT_SINE,
  EASE_IN_EXPO,
  EASE_OUT_EXPO,
  EASE_IN_OUT_EXPO,
  EASE_IN_CIRC,
  EASE_OUT_CIRC,
  EASE_IN_OUT_CIRC,
  EASE_IN_ELASTIC,
  EASE_OUT_ELASTIC,
  EASE_IN_OUT_ELASTIC,
  EASE_IN_BACK,
  EASE_OUT_BACK,
  EASE_IN_OUT_BACK,
  EASE_IN_BOUNCE,
  EASE_OUT_BOUNCE,
  EASE_IN_OUT_BOUNCE
};

/*
 * A style prop is a view property (x, y, width, height)
 * that is used by a style animation frame to modify a view's style
 * initial is the value that the property starts at
 * target is where it should end.
 * Alteratively, if is_delta is true, delta does ??? TODO what?
 */
typedef struct style_prop_t {
	unsigned int /* enum style_props */ name;
	double initial;
	// we can reuse target for delta
	union {
		double target;
		double delta;
	};
	bool is_delta;

	struct style_prop_t *prev;
	struct style_prop_t *next;
} style_prop;

/*
 * a frame is a unit of the animation.  Each frame is a specific
 * type of animation - style modification, wait, or function calling
 * resolved ?? TODO what?
 * If it's a style, frame, prop_head is the head of a linked list of
 * style properties and duration is the length.  Transition is the function
 * to use for easing.  TODO hook transition up to js
 * If it's a function call, cb is a wrapper of the function to call
 * A wait frame simply waits the duration.
 * TODO implement onTick and/or cbs with duration
 */
typedef struct frame_t {
	enum frame_type type;
	bool resolved;
	unsigned int duration;
	unsigned int id;
	unsigned int /* enum transitions */ transition;
	PERSISTENT_JS_OBJECT_WRAPPER cb;
	JS_OBJECT_WRAPPER onTick;
	style_prop *prop_head;

	struct frame_t *prev;
	struct frame_t *next;
} anim_frame;

/*
 * A view animation is the backing of a javascript animation.
 * It has a series of frames, stored in a linked list starting with
 * frame_head. The first frame in the list is the active one.
 * Status can be one of ?? TODO ?
 * elapsed is the length of time the animation has been running
 */
typedef struct view_animation_t {
	anim_frame *frame_head;
	struct timestep_view_t *view;
	unsigned int elapsed;
	bool is_scheduled;
	bool is_paused;

	struct view_animation_t *next;
	struct view_animation_t *prev;

	JS_OBJECT_WRAPPER js_anim;
} view_animation;


#endif // TIMESTEP_H

