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

#ifndef TEALEAF_CONTEXT_H
#define TEALEAF_CONTEXT_H

#include "geometry.h"
#include "rgba.h"
#include "core/tealeaf_canvas.h"
#include "core/texture_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODEL_VIEW_STACK_SIZE 64

extern matrix_3x3 tealeaf_context_projection_matrix;

typedef struct context_2d_t {
	tealeaf_canvas *canvas;
	int destTex;
	char *url;

	int width;
	int height;
	int backing_width;
	int backing_height;

	bool on_screen;
	matrix_3x3 proj_matrix;
	float globalAlpha[MODEL_VIEW_STACK_SIZE];
	int globalCompositeOperation[MODEL_VIEW_STACK_SIZE];
	matrix_3x3 modelView[MODEL_VIEW_STACK_SIZE];
	int mvp; // model view pointer
	rect_2d clipStack[MODEL_VIEW_STACK_SIZE];
	rgba filter_color;
	int filter_type;
} context_2d;

enum filter_mode {
    FILTER_NONE,
    FILTER_LINEAR_ADD,
    FILTER_MULTIPLY,
    FILTER_TINT
};

enum composite_mode {
    source_atop = 1337,
    source_in,
    source_out,
    source_over,
    destination_atop,
    destination_in,
    destination_out,
    destination_over,
    lighter,
    x_or,
    copy
};

int tealeaf_context_load_shader(char *vertex_shader_code, char *fragment_shader_code);
void tealeaf_context_update_viewport(context_2d *ctx, bool force);
void tealeaf_context_update_shader(context_2d *ctx, unsigned int shader_type, bool force);

context_2d *context_2d_get_onscreen();
context_2d *context_2d_new(tealeaf_canvas *canvas, const char *url, int destTex);
context_2d *context_2d_init(tealeaf_canvas *canvas, const char *url, int dest_tex, bool on_screen);

unsigned char *context_2d_read_pixels(context_2d *ctx);
char *context_2d_save_buffer_to_base64(context_2d *ctx, const char *image_type);

void context_2d_delete(context_2d *ctx);
void context_2d_resize(context_2d *ctx, int w, int h);
void context_2d_setGlobalAlpha(context_2d *ctx, float alpha);
float context_2d_getGlobalAlpha(context_2d *ctx);
void context_2d_setGlobalCompositeOperation(context_2d *ctx, int composite_mode);
int context_2d_getGlobalCompositeOperation(context_2d *ctx);
void context_2d_bind(context_2d *ctx);
void context_2d_setClip(context_2d *ctx, rect_2d clip);
void context_2d_save(context_2d *ctx);
void context_2d_restore(context_2d *ctx);
void context_2d_clear(context_2d *ctx);
void context_2d_loadIdentity(context_2d *ctx);
void context_2d_rotate(context_2d *ctx, float angle);
void context_2d_translate(context_2d *ctx, float x, float y);
void context_2d_scale(context_2d *ctx, float x, float y);
void context_2d_clearRect(context_2d *ctx, const rect_2d *rect);
void context_2d_fillRect(context_2d *ctx, const rect_2d *rect, const rgba *color);
void context_2d_fillText(context_2d *ctx, texture_2d *img, const rect_2d *srcRect, const rect_2d *destRect, float alpha);
void context_2d_flush(context_2d *ctx);
void context_2d_drawImage(context_2d *ctx, int srcTex, const char *url, const rect_2d *srcRect, const rect_2d *destRect);
void context_2d_draw_point_sprites(context_2d *ctx, const char *url, float point_size, float step_size, rgba *color, float x1, float y1, float x2, float y2);


void context_2d_add_filter(context_2d *ctx, rgba *color);
void context_2d_clear_filters(context_2d *ctx);
void context_2d_set_filter_type(context_2d *ctx, int filter_type);

void disable_scissor(context_2d *ctx);
void enable_scissor(context_2d *ctx);

void context_2d_setTransform(context_2d *ctx, double m11, double m12, double m21, double m22, double dx, double dy);
#ifdef __cplusplus
}
#endif

#endif
