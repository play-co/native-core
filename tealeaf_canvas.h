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

#ifndef TEALEAF_CANVAS_H
#define TEALEAF_CANVAS_H

#include "platform/gl.h"
#include "core/types.h"

typedef struct context_2d_t *context_2d_p;

typedef struct tealeaf_canvas_t {
	int framebuffer_width;
	int framebuffer_height;
	int framebuffer_offset_bottom;
	GLuint view_framebuffer;
	GLuint offscreen_framebuffer;
	bool should_resize;
	bool on_screen;
	context_2d_p onscreen_ctx;
	context_2d_p active_ctx;
} tealeaf_canvas;

#ifdef __cplusplus
extern "C" {
#endif

void tealeaf_canvas_bind_render_buffer(context_2d_p ctx);
void tealeaf_canvas_bind_texture_buffer(context_2d_p ctx);
void tealeaf_canvas_resize(int w, int h);
bool tealeaf_canvas_context_2d_bind(context_2d_p ctx);
void tealeaf_canvas_context_2d_rebind(context_2d_p ctx);

tealeaf_canvas *tealeaf_canvas_get();
void tealeaf_canvas_init(int framebuffer_name);

#ifdef __cplusplus
}
#endif

#endif // TEALEAF_CANVAS_H
