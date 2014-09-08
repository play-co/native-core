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

/**
 * @file	 draw_textures.c
 * @brief
 */
#include "core/draw_textures.h"
#include <sys/time.h>
#include "core/tealeaf_context.h"
#include "core/tealeaf_shaders.h"
#include "core/log.h"
#include "core/graphics_utils.h"
#include "platform/gl.h"
#include <math.h>

#define DRAW_TEXTURES_PROFILE 0
#define MAX_BUFFER_SIZE 1024


static int lastName = -1;
static int bufSize = 0;
static float lastOpacity = 1;
static int last_composite_op = 0;
static int last_filter_type = 0;
static rgba last_filter_color = {0, 0, 0, 0};
typedef struct bufobj_t {
    float srcX1;
    float srcY1;
    float destX1;
    float destY1;
    float srcX2;
    float srcY2;
    float destX2;
    float destY2;
    float srcX3;
    float srcY3;
    float destX3;
    float destY3;
} bufobj;

static bufobj buffer[MAX_BUFFER_SIZE];
/**
 * @name	draw_textures_item
 * @brief	takes the given options and queues a texture to be drawn.
 *			this may also trigger a draw_textures_flush if options warranting
 *			a flush are found.
 * @param	model_view - (matrix_3x3) currently used modelview
 * @param	name - (int) gl texture id
 * @param	src_width - (int) width of the source texture
 * @param	src_height - (int) height of the source texture
 * @param	orig_width - (deprecated)
 * @param	orig_height - (deprecated)
 * @param	src - (rect_2d) source rectangle to pull pixels off of from the given texture
 * @param	dest - (rect_2d) destination rectangle to draw to
 * @param	clip - (rect_2d) current clipping rectangle
 * @param	opacity - (float) the global opacity to draw with
 * @param	composite_op - (int) coposite operation to use for rendering
 * @param	filter_color - (rgba*) the color object being used by the filter
 * @param	filter_type - (int) the type of filter being used currently
 * @retval	NONE
 */
void draw_textures_item(context_2d *ctx, const matrix_3x3 *model_view, int name, int src_width, int src_height, int orig_width, int orig_height, rect_2d src, rect_2d dest, rect_2d clip, float opacity, int composite_op, rgba *filter_color, int filter_type) {

    //ignore this item if clip height is 0
    if (clip.height == 0 || clip.width == 0) {
        return;
    }

    if (name != lastName || bufSize + 2 >= MAX_BUFFER_SIZE || lastOpacity != opacity || composite_op != last_composite_op  || !rgba_equals(&last_filter_color, filter_color) || last_filter_type != filter_type) {
        // TODO: PERFORMANCE: could send opacity to GPU too by interleaving a buffered color array
        draw_textures_flush();
        lastName = name;
        lastOpacity = opacity;
        last_composite_op = composite_op;
        last_filter_color.r = filter_color->r;
        last_filter_color.g = filter_color->g;
        last_filter_color.b = filter_color->b;
        last_filter_color.a = filter_color->a;
        last_filter_type = filter_type;
    }

    bufSize += 2;
    bufobj *o = buffer + bufSize - 2;
    bufobj *o2 = o + 1;
    float sMin, tMin, sMax, tMax;
    sMin = src.x / (float) src_width,
    tMin = src.y / (float)src_height,
    sMax = (src.x + src.width) / (float)src_width,
    tMax = (src.y + src.height) / (float)src_height;

    o->srcX1 = sMin;
    o->srcY1 = tMax;
    o->srcX2 = sMax;
    o->srcY2 = tMax;
    o->srcX3 = sMin;
    o->srcY3 = tMin;
    o2->srcX1 = sMax;
    o2->srcY1 = tMax;
    o2->srcX2 = sMax;
    o2->srcY2 = tMin;
    o2->srcX3 = sMin;
    o2->srcY3 = tMin;
    float x1, y1, x2, y2, x3, y3, x4, y4;
    matrix_3x3_multiply(model_view, &dest, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4);
    o->destX1 = x4;
    o->destY1 = y4;
    o->destX2 = x3;
    o->destY2 = y3;
    o->destX3 =  x1;
    o->destY3 = y1;
    o2->destX1 = x3;
    o2->destY1 = y3;
    o2->destX2 = x2;
    o2->destY2 = y2;
    o2->destX3 = x1;
    o2->destY3 = y1;

    //if the last composite operation is one which requires
    //being applied to the full canvas, do full canvas composite
    //preparement
    if (is_full_canvas_composite_operation(last_composite_op)) {
        set_up_full_compositing(ctx, (int)x1, (int)y1, (int)(x2 - x1), (int)(y3 - y1), last_composite_op);
        draw_textures_flush();
    }
}

#if DRAW_TEXTURES_PROFILE
struct timeval lastFlush, prevTime, now;
#endif

/**
 * @name	draw_textures_flush
 * @brief	renders all the textures queued to draw
 * @retval	NONE
 */
void draw_textures_flush() {
    if (bufSize <= 0) {
        return;
    }

    if (lastOpacity > 0) {
        int stride = sizeof(float) * 4;

        apply_composite_operation(last_composite_op);

        if (use_single_shader) {
            tealeaf_shaders_bind(PRIMARY_SHADER);
            GLTRACE(glUniform4f(global_shaders[current_shader].draw_color, lastOpacity, lastOpacity, lastOpacity, lastOpacity));
        } else {
            //TODO: implement filters using filter_type on views properly
            if (last_filter_type == FILTER_NONE) {
                tealeaf_shaders_bind(PRIMARY_SHADER);
                GLTRACE(glUniform4f(global_shaders[current_shader].draw_color, lastOpacity, lastOpacity, lastOpacity, lastOpacity));
            } else if (last_filter_type == FILTER_LINEAR_ADD) {
                float r = last_filter_color.r * last_filter_color.a;
                float g = last_filter_color.g * last_filter_color.a;
                float b = last_filter_color.b * last_filter_color.a;
                tealeaf_shaders_bind(LINEAR_ADD_SHADER);
                GLTRACE(glUniform4f(global_shaders[current_shader].add_color, r, g, b, 0));
                GLTRACE(glUniform4f(global_shaders[current_shader].draw_color, lastOpacity, lastOpacity, lastOpacity, lastOpacity));
            } else if (last_filter_type == FILTER_MULTIPLY) {
                float r = 1 + (last_filter_color.r - 1) * last_filter_color.a;
                float g = 1 + (last_filter_color.g - 1) * last_filter_color.a;
                float b = 1 + (last_filter_color.b - 1) * last_filter_color.a;
                tealeaf_shaders_bind(PRIMARY_SHADER);
                GLTRACE(glUniform4f(global_shaders[current_shader].draw_color, r * lastOpacity, g * lastOpacity, b * lastOpacity, lastOpacity));
            } else if (last_filter_type == FILTER_TINT) {
                float a = last_filter_color.a;
                float t = 1 - a;
                float r = last_filter_color.r * a;
                float g = last_filter_color.g * a;
                float b = last_filter_color.b * a;
                tealeaf_shaders_bind(LINEAR_ADD_SHADER);
                GLTRACE(glUniform4f(global_shaders[current_shader].add_color, r, g, b, 0));
                GLTRACE(glUniform4f(global_shaders[current_shader].draw_color, lastOpacity * t, lastOpacity * t, lastOpacity * t, lastOpacity));
            }

        }

        GLTRACE(glActiveTexture(GL_TEXTURE0));
        GLTRACE(glBindTexture(GL_TEXTURE_2D, lastName));
        GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GLTRACE(glVertexAttribPointer(global_shaders[current_shader].vertex_coords, 2, GL_FLOAT, GL_FALSE, stride, ((float *) buffer) + 2));
        //TexCoord0, XY (Also called ST. Also called UV), FLOAT.
        GLTRACE(glVertexAttribPointer(global_shaders[current_shader].tex_coords, 2, GL_FLOAT, GL_FALSE, stride, buffer));
#if DRAW_TEXTURES_PROFILE
        gettimeofday(&prevTime, NULL);
#endif
        GLTRACE(glDrawArrays(GL_TRIANGLES, 0, 3 * bufSize));
#if DRAW_TEXTURES_PROFILE
        gettimeofday(&now, NULL);
        LOG("{drawtex} Flush: %d %d %ld %ld\n", bufSize / 2, lastName,
            (now.tv_usec - prevTime.tv_usec),
            (now.tv_usec - lastFlush.tv_usec));
        lastFlush = now;
#endif
    }

    bufSize = 0;
}
