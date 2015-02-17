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
 * @file	 tealeaf_context.c
 * @brief
 */
#include "core/tealeaf_context.h"
#include "core/tealeaf_shaders.h"
#include "core/log.h"
#include "core/draw_textures.h"
#include "core/texture_2d.h"
#include "core/texture_manager.h"
#include "core/geometry.h"
#include "core/image_writer.h"
#include "core/graphics_utils.h"
#include <math.h>
#include <stdlib.h>

#define GET_MODEL_VIEW_MATRIX(ctx) (&ctx->modelView[ctx->mvp])
#define GET_CLIPPING_BOUNDS(ctx) (&ctx->clipStack[ctx->mvp])
#define IS_SCISSOR_ENABLED(ctx) (GET_CLIPPING_BOUNDS(ctx)->width >= 0)

static rect_2d last_scissor_rect;


/**
 * @name	tealeaf_context_set_proj_matrix
 * @brief	set the context's ortho projection using properties on the context
 * @param	ctx - (context_2d *) context on which to set the ortho projection on
 * @retval	NONE
 */
void tealeaf_context_set_proj_matrix(context_2d *ctx) {
    int width = ctx->backing_width;
    int height = ctx->backing_height;

    if (!ctx->on_screen) {
        matrix_3x3_ortho(&ctx->proj_matrix, 0, width, height, 0);
    } else {
        matrix_3x3_ortho(&ctx->proj_matrix, 0, width, 0, height);
    }

    matrix_3x3_transpose(&ctx->proj_matrix);
}

/**
 * @name	tealeaf_context_update_viewport
 * @brief	updates the gl viewport using the context's properties
 * @param	ctx - (context_2d *) context to update the viewport by
 * @param	force - (bool) force an updating of shaders
 * @retval	NONE
 */
void tealeaf_context_update_viewport(context_2d *ctx, bool force) {
    tealeaf_context_update_shader(ctx, DRAWING_SHADER, force);
    tealeaf_context_update_shader(ctx, PRIMARY_SHADER, force);
    tealeaf_context_update_shader(ctx, FILL_RECT_SHADER, force);
    tealeaf_context_update_shader(ctx, LINEAR_ADD_SHADER, force);
    GLTRACE(glViewport(0, 0, ctx->backing_width, ctx->backing_height));
}

/**
 * @name	print_model_view
 * @brief	pretty prints the model view matrix contained inside the given context
 * @param	ctx - (context_2d *) context to print the model view matrix from
 * @param	i - (int) which model view matrix to print from the stack
 * @retval	NONE
 */
void print_model_view(context_2d *ctx, int i) {
    matrix_3x3 m __attribute__((unused)) = ctx->modelView[i];
    LOG("%f %f %f \n %f %f %f\n %f %f %f\n",
        m.m00, m.m01, m.m02,
        m.m10, m.m11, m.m12,
        m.m20, m.m21, m.m22);
}

/**
 * @name	disable_scissor
 * @brief	disables the use of glScissors
 * @param	ctx - deprecated
 * @retval	NONE
 */
void disable_scissor(context_2d *ctx) {
    draw_textures_flush();
    last_scissor_rect.x = last_scissor_rect.y = 0;
    last_scissor_rect.width = last_scissor_rect.height = -1;
    GLTRACE(glDisable(GL_SCISSOR_TEST));
}

/**
 * @name	enable_scissor
 * @brief   enables the use of glScissors using proprties from the given context
 * @param	ctx - (context_2d *)
 * @retval	NONE
 */
void enable_scissor(context_2d *ctx) {
    rect_2d *bounds = GET_CLIPPING_BOUNDS(ctx);

    if (rect_2d_equals(&last_scissor_rect, bounds)) {
        return;
    }

    draw_textures_flush();
    last_scissor_rect.x = bounds->x;
    last_scissor_rect.y = bounds->y;
    last_scissor_rect.width = bounds->width;
    last_scissor_rect.height = bounds->height;
    GLTRACE(glScissor((int) bounds->x, (int) bounds->y, (int) bounds->width, (int) bounds->height));
    GLTRACE(glEnable(GL_SCISSOR_TEST));
}

/**
 * @name	context_2d_init
 * @brief	init's and returns a context_2d pointer with the given options
 * @param	canvas - (tealeaf_canvas *) canvas to use as the context's canvas
 * @param	url - (const char *) name of the canvas
 * @param	dest_tex - (int)
 * @param	on_screen - (bool) whether this should be an onscreen or offscreen context
 * @retval	context_2d* - pointer to the created context
 */
context_2d *context_2d_init(tealeaf_canvas *canvas, const char *url, int dest_tex, bool on_screen) {
    context_2d *ctx = (context_2d *) malloc(sizeof(context_2d));
    ctx->mvp = 0;
    ctx->globalAlpha[0] = 1;
    ctx->globalCompositeOperation[0] = 0;
    ctx->destTex = dest_tex;
    ctx->on_screen = on_screen;
    ctx->filter_color.r = 0.0;
    ctx->filter_color.g = 0.0;
    ctx->filter_color.b = 0.0;
    ctx->filter_color.a = 0.0;
    ctx->filter_type = FILTER_NONE;

    if (!on_screen) {
        texture_2d *tex = texture_manager_get_texture(texture_manager_get(), url);

        if (tex) {
            ctx->backing_width = tex->width;
            ctx->backing_height = tex->height;
            ctx->width = tex->originalWidth;
            ctx->height = tex->originalHeight;
            tealeaf_context_set_proj_matrix(ctx);
            tex->ctx = ctx;
        }
    } else {
        ctx->width = 0;
        ctx->height = 0;
    }

    ctx->canvas = canvas;
    size_t len = strlen(url);
    ctx->url = (char *)malloc(sizeof(char) * (len + 1));
    strlcpy(ctx->url, url, len + 1);
    ctx->clipStack[0].x = 0;
    ctx->clipStack[0].y = 0;
    ctx->clipStack[0].width = -1;
    ctx->clipStack[0].height = -1;
    matrix_3x3_identity(&ctx->modelView[0]);
    context_2d_clear(ctx);
    return ctx;
}

/**
 * @name	context_2d_get_onscreen
 * @brief
 * @retval	context_2d* -
 */
context_2d *context_2d_get_onscreen() {
    tealeaf_canvas *canvas = tealeaf_canvas_get();
    return canvas->onscreen_ctx;
}

// TODO: this should return dest_tex and allocate the texture itself...
/**
 * @name	context_2d_new
 * @brief	create's and returns a new context
 * @param	canvas - (tealeaf_canvas *)
 * @param	url - (const char *) name of the canvas
 * @param	dest_tex - (int)
 * @retval	context_2d* - pointer to the created context
 */
context_2d *context_2d_new(tealeaf_canvas *canvas, const char *url, int dest_tex) {
    context_2d *ctx;

    if (!strcmp(url, "onscreen")) {
        ctx = context_2d_get_onscreen();
    } else {
        ctx = context_2d_init(canvas, url, dest_tex, false);
    }

    return ctx;
}

/**
   Uses glReadPixels to read the bytes of the given context's drawing buffer into
   an unsigned char array

   @param	ctx the given context2d
   @return 	an unsigned char array containing the bytes of ctx's draw buffer
**/
unsigned char *context_2d_read_pixels(context_2d *ctx) {
    //must flush before reading as canvas may not be ready
    //to be read from
    draw_textures_flush();
    unsigned char *buffer = NULL;
    buffer = (unsigned char *)malloc(sizeof(unsigned char) * 4 * ctx->width * ctx->height);
    GLTRACE(glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, buffer));
    return buffer;
}

/**
   Saves the given context_2d's buffer to a file of the given filetype

   @param	ctx the given context2d
   @param	file_name  filename to save the buffer to
   @return 	void
**/
char *context_2d_save_buffer_to_base64(context_2d *ctx, const char *image_type) {

    //bind offscreen buffer to gl frame buffer
    tealeaf_canvas_context_2d_bind(ctx);
    unsigned char *buffer = (unsigned char*)context_2d_read_pixels(ctx);
    //opengGL gives this as RGBA, Need to switch to
    //BGRA which will be interpreted as ARGB in Java
    //becuase of the endianess difference between Java/C
//	int i;
//	for(i = 0; i < ctx->width * ctx->height * 4; i+=4) {
//		char r = buffer[i];
//		buffer[i] = buffer[i + 2];
//		buffer[i + 2] = r;
//	}
    char *buf = (char*)write_image_to_base64(image_type, buffer, ctx->width, ctx->height, 4);
    tealeaf_canvas_context_2d_bind(context_2d_get_onscreen());
    free(buffer);
    return buf;
}
/**
 * @name	context_2d_delete
 * @brief	frees the given context
 * @param	ctx - (context_2d *) context to delete
 * @retval	NONE
 */
void context_2d_delete(context_2d *ctx) {
    texture_2d *tex = texture_manager_get_texture(texture_manager_get(), (char *)ctx->url);

    if (tex) {
        texture_manager_free_texture(texture_manager_get(), tex);
    }

    free(ctx->url);
    free(ctx);
}

/**
 * @name    context_2d_resize
 * @brief   resizes the given context to the given width / height
 * @param   ctx - (context_2d *) context to resize
 * @param   width - (int) width to resize to
 * @param   height - (int) height to resize to
 * @retval  NONE
 */
void context_2d_resize(context_2d *ctx, int width, int height) {
    if (ctx->on_screen) {
        ctx->backing_width = width;
        ctx->backing_height = height;
        ctx->width = width;
        ctx->height = height;
        tealeaf_context_set_proj_matrix(ctx);
    } else {
        texture_2d *tex = texture_manager_get_texture(texture_manager_get(), ctx->url);
        tex = texture_manager_resize_texture(texture_manager_get(), tex, width, height);

        free(ctx->url);
        size_t len = strlen(tex->url);
        ctx->url = (char *)malloc(sizeof(char) * (len + 1));
        strlcpy(ctx->url, tex->url, len + 1);

        ctx->backing_width = tex->width;
        ctx->backing_height = tex->height;
        ctx->width = tex->originalWidth;
        ctx->height = tex->originalHeight;

        tealeaf_canvas_context_2d_rebind(ctx);
    }
}

/**
 * @name	context_2d_setGlobalAlpha
 * @brief	sets the given global alpha on the context
 * @param	ctx - (context_2d *) context to set the global alpha on
 * @param	alpha - (float) the global alpha
 * @retval	NONE
 */
void context_2d_setGlobalAlpha(context_2d *ctx, float alpha) {
    ctx->globalAlpha[ctx->mvp] = alpha;
}
/**
 * @name	context_2d_getGlobalAlpha
 * @brief	get's the global alpha from the given context
 * @param	ctx - (context_2d *) context to get the global alpha from
 * @retval	float - the global alpha gotten from the context
 */
float context_2d_getGlobalAlpha(context_2d *ctx) {
    return ctx->globalAlpha[ctx->mvp];
}

/**
 * @name	context_2d_bind
 * @brief	bind's the given context to gl and enables / disables scissors
 * @param	ctx - (context_2d *) context to bind
 * @retval	NONE
 */
void context_2d_bind(context_2d *ctx) {
    if (tealeaf_canvas_context_2d_bind(ctx)) {
        if (IS_SCISSOR_ENABLED(ctx)) {
            enable_scissor(ctx);
        } else {
            disable_scissor(ctx);
        }
    }
}

void context_2d_setGlobalCompositeOperation(context_2d *ctx, int composite_mode) {
    ctx->globalCompositeOperation[ctx->mvp] = composite_mode;
}

int context_2d_getGlobalCompositeOperation(context_2d *ctx) {
    return ctx->globalCompositeOperation[ctx->mvp];
}


//TODO: Rename
/**
 * @name	context_2d_add_filter
 * @brief	puts the given filter on the given context
 * @param	ctx - (context_2d *) context to add the filter onto
 * @param	color - (rgba *) color to set the filter color to on the context
 * @retval	NONE
 */
void context_2d_add_filter(context_2d *ctx, rgba *color) {
    ctx->filter_color.r = color->r;
    ctx->filter_color.g = color->g;
    ctx->filter_color.b = color->b;
    ctx->filter_color.a = color->a;
}

/**
 * @name	context_2d_clear_filters
 * @brief	clear's filters on the the given context
 * @param	ctx - (context_2d *) context to clear filters on
 * @retval	NONE
 */
void context_2d_clear_filters(context_2d *ctx) {
    ctx->filter_type = 0;
    ctx->filter_color.r = 0.0;
    ctx->filter_color.g = 0.0;
    ctx->filter_color.b = 0.0;
    ctx->filter_color.a = 0.0;
}

/**
 * @name	context_2d_set_filter_type
 * @brief	sets the filter type on the given context
 * @param	ctx - (context_2d *) context to set the filter type on
 * @param	filter_type - (int) a filter enum
 * @retval	NONE
 */
void context_2d_set_filter_type(context_2d *ctx, int filter_type) {
    ctx->filter_type = filter_type;
}

/**
 * @name	context_2d_setClip
 * @brief	sets the clipping rectangle on the given context
 * @param	ctx - (context_2d *) context to set the clipping rectangle on
 * @param	clip - (rect_2d) the clipping rectangle
 * @retval	NONE
 */
void context_2d_setClip(context_2d *ctx, rect_2d clip) {
    matrix_3x3 *modelView = GET_MODEL_VIEW_MATRIX(ctx);

#ifdef MATRIX_3x3_ALLOW_SKEW
    // TODO: Can this be done more efficiently?
    float clip_x, clip_y, clip_w, clip_h;
    float x1, y1, x2, y2, x3, y3, x4, y4;

    matrix_3x3_multiply(modelView, clip.x, clip.y, &x1, &y1);
    matrix_3x3_multiply(modelView, clip.x + clip.width, clip.y + clip.height, &x2, &y2);
    matrix_3x3_multiply(modelView, clip.x, clip.y + clip.height, &x3, &y3);
    matrix_3x3_multiply(modelView, clip.x + clip.width, clip.y, &x4, &y4);

    clip_x = x1;
    if (x2 < clip_x) {
        clip_x = x2;
    }
    if (x3 < clip_x) {
        clip_x = x3;
    }
    if (x4 < clip_x) {
        clip_x = x4;
    }

    clip_y = y1;
    if (y2 < clip_y) {
        clip_y = y2;
    }
    if (y3 < clip_y) {
        clip_y = y3;
    }
    if (y4 < clip_y) {
        clip_y = y4;
    }

    clip_w = x1;
    if (x2 > clip_w) {
        clip_w = x2;
    }
    if (x3 > clip_w) {
        clip_w = x3;
    }
    if (x4 > clip_w) {
        clip_w = x4;
    }

    clip_h = y1;
    if (y2 > clip_h) {
        clip_h = y2;
    }
    if (y3 > clip_h) {
        clip_h = y3;
    }
    if (y4 > clip_h) {
        clip_h = y4;
    }

    clip.x = clip_x;
    clip.y = clip_y;
    clip.width = clip_w - clip_x;
    clip.height = clip_h - clip_y;
#else
    float clip0x = clip.x, clip0y = clip.y;
    float clip1x = clip0x + clip.width, clip1y = clip0y + clip.height;

    // float x1, y1, x2, y2, x3, y3, x4, y4;
    // x1 = clip0x * modelView->m00 + clip0y * modelView->m01 + modelView->m02;
    // y1 = clip0x * modelView->m10 + clip0y * modelView->m11 + modelView->m12;
    // x2 = clip1x * modelView->m00 + clip0y * modelView->m01 + modelView->m02;
    // y2 = clip1x * modelView->m10 + clip0y * modelView->m11 + modelView->m12;
    // x3 = clip1x * modelView->m00 + clip1y * modelView->m01 + modelView->m02;
    // y3 = clip1x * modelView->m10 + clip1y * modelView->m11 + modelView->m12;
    // x4 = clip0x * modelView->m00 + clip1y * modelView->m01 + modelView->m02;
    // y4 = clip0x * modelView->m10 + clip1y * modelView->m11 + modelView->m12;

    float m00 = modelView->m00, m01 = modelView->m01, m10 = modelView->m10, m11 = modelView->m11;
    float a = clip0x * m00;
    float b = clip1x * m10;
    float c = clip0y * m11;
    float d = clip0y * m01;
    float e = clip1x * m00;
    float f = clip1y * m01;
    float g = clip0x * m10;
    float h = clip1y * m11;

    // If x1 < x2,
    if ((clip0x < clip1x) ^ (m00 < 0)) {
        // If x2 < x3,
        if ((clip0y < clip1y) ^ (m01 < 0)) {
            // (x1, y2) -> (x3, y4)
            clip.x = a + d;
            clip.y = b + c;
            clip.width = e + f;
            clip.height = g + h;
        } else {
            // (x4, y1) -> (x2, y3)
            clip.x = a + f;
            clip.y = g + c;
            clip.width = e + d;
            clip.height = b + h;
        }
    } else {
        // If x2 < x3,
        if ((clip0y < clip1y) ^ (m01 < 0)) {
            // (x2, y3) -> (x4, y1)
            clip.x = e + d;
            clip.y = b + h;
            clip.width = a + f;
            clip.height = g + c;
        } else {
            // (x3, y4) -> (x1, y2)
            clip.x = e + f;
            clip.y = g + h;
            clip.width = a + d;
            clip.height = b + c;
        }
    }

    clip.width -= clip.x;
    clip.height -= clip.y;
    clip.x += modelView->m02;
    clip.y += modelView->m12;
#endif

    // Clip with screen bounds
    if (clip.x < 0) {
        clip.width += clip.x;
        if (clip.width < 0) {
            clip.width = 0;
        }
        clip.x = 0;
    }
    if (clip.y < 0) {
        clip.height += clip.y;
        if (clip.height < 0) {
            clip.height = 0;
        }
        clip.y = 0;
    }

    // If entirely clipped,
    if (clip.width <= 0 || clip.height <= 0) {
        clip.x = clip.y = clip.width = clip.height = 0;
    } else {
        // Lookup parent bounds
        int i = ctx->mvp - 1;
        rect_2d ctx_clip;
        ctx_clip.x = ctx->clipStack[i].x;
        ctx_clip.y = ctx->clipStack[i].y;
        ctx_clip.width = ctx->clipStack[i].width;
        ctx_clip.height = ctx->clipStack[i].height;

        // If context is on screen,
        if (ctx->on_screen) {
            // Flip to frame buffer sense
            ctx_clip.y = -ctx_clip.y +  ctx->canvas->framebuffer_height + ctx->canvas->framebuffer_offset_bottom - ctx_clip.height;
        }

        // If parent is clipping,
        if (ctx_clip.width > -1) {
            // Calculate (x1, y1) for new and old clip regions
            float clip1x = clip.x + clip.width, clip1y = clip.y + clip.height;
            float ctx1x = ctx_clip.x + ctx_clip.width, ctx1y = ctx_clip.y + ctx_clip.height;

            // If new clip is entirely outside parent,
            if (clip.x >= ctx1x || clip1x <= ctx_clip.x ||
                    clip.y >= ctx1y || clip1y <= ctx_clip.y) {
                // Empty clip region
                clip.x = clip.y = clip.width = clip.height = 0;
            } else {
                // Trim new clip with parent
                clip.x = ctx_clip.x > clip.x ? ctx_clip.x : clip.x;
                clip.y = ctx_clip.y > clip.y ? ctx_clip.y : clip.y;
                clip.width = (ctx1x < clip1x ? ctx1x : clip1x) - clip.x;
                clip.height = (ctx1y < clip1y ? ctx1y : clip1y) - clip.y;
            }
        }

        // scissor is with respect to lower-left corner
        // activeFrameBufferHeight is the height of the off-screen buffer
        // activeFrameBufferOffsetBottom -- the viewport actually goes past the bottom of the texture
        //   to the nearest power of two, so when we convert to y-coordinates from the lower-left viewport
        //   corner, we need to add the offsetBottom to get to the bottom of the viewable texture
        if (ctx->on_screen && clip.height > 0) {
            // Flip from frame buffer sense
            clip.y = ctx->canvas->framebuffer_height - (clip.height + clip.y) + ctx->canvas->framebuffer_offset_bottom;
        }
    }

    rect_2d bounds = {
        clip.x, clip.y,
        clip.width, clip.height
    };

    if (rect_2d_equals(GET_CLIPPING_BOUNDS(ctx), &bounds)) {
        return;
    }

    *GET_CLIPPING_BOUNDS(ctx) = bounds;
    enable_scissor(ctx);
}

/**
 * @name	context_2d_save
 * @brief	save's the context's pertinent values to their global stacks
 * @param	ctx - (context_2d *) context to save
 * @retval	NONE
 */
void context_2d_save(context_2d *ctx) {
    int mvp = ctx->mvp + 1;

    // If stack size is not exceeded,
    if (mvp >= MODEL_VIEW_STACK_SIZE) {
        LOG("{context} WARNING: Stack size exceeded. View hierarchy may not exceed %d levels", MODEL_VIEW_STACK_SIZE);
    } else {
        ctx->mvp = mvp;
        ctx->globalAlpha[mvp] = ctx->globalAlpha[mvp - 1];
        ctx->modelView[mvp] = ctx->modelView[mvp - 1];
        ctx->clipStack[mvp] = ctx->clipStack[mvp - 1];
        ctx->globalCompositeOperation[mvp] = ctx->globalCompositeOperation[mvp - 1];
    }
}

/**
 * @name	context_2d_restore
 * @brief	pop's off the global properties stacks and resets glScissors
 * @param	ctx - (context_2d *) context to restore
 * @retval	NONE
 */
void context_2d_restore(context_2d *ctx) {
    int mvp = ctx->mvp - 1;

    // If stack still has items on it,
    if (mvp >= 0) {
        ctx->mvp = mvp;

        if (!rect_2d_equals(&ctx->clipStack[mvp], &ctx->clipStack[mvp + 1])) {
            if (ctx->clipStack[mvp].width == -1) {
                disable_scissor(ctx);
            } else {
                enable_scissor(ctx);
            }
        }
    }
}

//TODO: this should only do a glClear with proper clear color settings
/**
 * @name	context_2d_clear
 * @brief
 * @param	ctx - (context_2d *)
 * @retval	NONE
 */
void context_2d_clear(context_2d *ctx) {
    draw_textures_flush();
    context_2d_bind(ctx);
    GLTRACE(glClearColor(0, 0, 0, 0));
    GLTRACE(glClear(GL_COLOR_BUFFER_BIT));
}

/**
 * @name	context_2d_loadIdentity
 * @brief	loads the identity matrix into the given context's model view matrix
 * @param	ctx - (context_2d *) context to load the identity matrix into
 * @retval	NONE
 */
void context_2d_loadIdentity(context_2d *ctx) {
    matrix_3x3_identity(GET_MODEL_VIEW_MATRIX(ctx));
}

/**
 * @name	context_2d_rotate
 * @brief	rotates the given context by given options
 * @param	ctx - (context_2d *) context to rotate
 * @param	angle - (float) angle to rotate by
 * @retval	NONE
 */
void context_2d_rotate(context_2d *ctx, float angle) {
    if (angle != 0) {
        matrix_3x3_rotate(GET_MODEL_VIEW_MATRIX(ctx), angle);
    }
}

/**
 * @name	context_2d_translate
 * @brief	translate's the given context by given options
 * @param	ctx - (context_2d *) context to translate
 * @param	x - (float) amount along the x-axis to translate by
 * @param	y - (float) amount along the y-axis to translate by
 * @retval	NONE
 */
void context_2d_translate(context_2d *ctx, float x, float y) {
    if (x != 0 || y != 0) {
        matrix_3x3_translate(GET_MODEL_VIEW_MATRIX(ctx), x, y);
    }
}

/**
 * @name	context_2d_draw_point_sprites
 * @brief	Draws pointsprites using the given options (in batch along a line)
 * @param	ctx - (context_2d *) context to draw to
 * @param	url - (const char *) name of the texture to draw from
 * @param	point_size - (float) point sprite size
 * @param	step_size - (float) step size to take between drawn pointsprites
 * @param	color - (rgba *) color to draw with
 * @param	x1 - (float) starting x-coordinate to draw along
 * @param	y1 - (float) starting y-coordinate to draw along
 * @param	x2 - (float) ending x-coordinate to draw along
 * @param	y2 - (float) ending y-coordinate to draw along
 * @retval	NONE
 */
void context_2d_draw_point_sprites(context_2d *ctx, const char *url, float point_size, float step_size, rgba *color, float x1, float y1, float x2, float y2) {
    draw_textures_flush();
    context_2d_bind(ctx);
    texture_2d *tex = texture_manager_load_texture(texture_manager_get(), url);

    // If texture is not finished loading,
    if (!tex || !tex->loaded) {
        return;
    }

    static GLfloat     *vertex_buffer = NULL;
    static unsigned int vertex_max = 64;
    tealeaf_shaders_bind(DRAWING_SHADER);
    matrix_3x3_multiply_m_f_f_f_f(GET_MODEL_VIEW_MATRIX(ctx), x1, y1, &x1, &y1);
    matrix_3x3_multiply_m_f_f_f_f(GET_MODEL_VIEW_MATRIX(ctx), x2, y2, &x2, &y2);

    // Allocate vertex array buffer
    if (vertex_buffer == NULL) {
        vertex_buffer = malloc(vertex_max * 2 * sizeof(GLfloat));
    }

    // Add points to the buffer so there are drawing points every X pixels
    unsigned int count = ceilf(sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) / step_size);

    if (count < 1) {
        count = 1;
    }

    unsigned int vertex_count = 0;
    unsigned int i;

    for (i = 0; i < count; ++i) {
        if (vertex_count == vertex_max) {
            vertex_max = 2 * vertex_max;
            vertex_buffer = realloc(vertex_buffer, vertex_max * 2 * sizeof(GLfloat));
        }

        vertex_buffer[2 * vertex_count + 0] = x1 + (x2 - x1) * ((GLfloat)i / (GLfloat)count);
        vertex_buffer[2 * vertex_count + 1] = y1 + (y2 - y1) * ((GLfloat)i / (GLfloat)count);
        vertex_count += 1;
    }

    GLTRACE(glActiveTexture(GL_TEXTURE0));
    GLTRACE(glBindTexture(GL_TEXTURE_2D, tex->name));
    GLTRACE(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    // Render the vertex array
    GLTRACE(glUniform1f(global_shaders[DRAWING_SHADER].point_size, point_size));
    GLTRACE(glVertexAttribPointer(global_shaders[DRAWING_SHADER].vertex_coords, 2, GL_FLOAT, GL_FALSE, 0, (float *) vertex_buffer));
    float alpha = color->a * ctx->globalAlpha[ctx->mvp];
    GLTRACE(glUniform4f(global_shaders[DRAWING_SHADER].draw_color, alpha * color->r, alpha * color->g, alpha * color->b, alpha));
    GLTRACE(glDrawArrays(GL_POINTS, 0, vertex_count));
    tealeaf_shaders_bind(PRIMARY_SHADER);
}

/**
 * @name	context_2d_scale
 * @brief	scales the given context by given options
 * @param	ctx - (context_2d *) context to scale
 * @param	x - (float) amount along the x-coordinate to scale
 * @param	y - (float) amount along the x-coordinate to scale
 * @retval	NONE
 */
void context_2d_scale(context_2d *ctx, float x, float y) {
    matrix_3x3_scale(GET_MODEL_VIEW_MATRIX(ctx), x, y);
}

/**
 * @name	context_2d_clearRect
 * @brief	clears the given rect on the given context
 * @param	ctx - (context_2d *) context to clear a rect from
 * @param	rect - (const rect_2d *) rect to clear from the context
 * @retval	NONE
 */
void context_2d_clearRect(context_2d *ctx, const rect_2d *rect) {
    rgba color = { 0, 0, 0, 0 };
    context_2d_fillRect(ctx, rect, &color);
}

/**
 * @name	tealeaf_context_update_shader
 * @brief	updates the given shader type using properties from the context
 * @param	ctx - (context_2d *) context to use properties from
 * @param	shader_type - (unsigned int) shader to update
 * @param	force - (bool) force update of the shader
 * @retval	NONE
 */

void tealeaf_context_update_shader(context_2d *ctx, unsigned int shader_type, bool force) {
    int width = ctx->backing_width;
    int height = ctx->backing_height;
    tealeaf_shader *shader = &global_shaders[shader_type];
    matrix_4x4 m;
    matrix_3x3 *proj;

    if (shader->last_width != width || shader->last_height != height || force) {
        //Need to copy the 3x3 projection matrix into a 4x4 matrix since that
        //is the form used in the shader. Note that in the future the shaders
        //could be changed to use 3x3 matrices rather than 4x4
        proj = &ctx->proj_matrix;
        m.m00 = proj->m00;
        m.m01 = proj->m01;
        m.m02 = 0;
        m.m03 = proj->m02;
        m.m10 = proj->m10;
        m.m11 = proj->m11;
        m.m12 = 0;
        m.m13 = proj->m12;
        m.m20 = 0;
        m.m21 = 0;
        m.m22 = 1;
        m.m23 = 0;
        m.m30 = proj->m20;
        m.m31 = proj->m21;
        m.m32 = 0;
        m.m33 = proj->m22;
        GLTRACE(glUseProgram(shader->program));
        GLTRACE(glUniformMatrix4fv(shader->proj_matrix, 1, false, (float *) &m));
        shader->last_width = width;
        shader->last_height = height;
        GLTRACE(glUseProgram(global_shaders[current_shader].program));
    }
}


/**
 * @name	context_2d_fillRect
 * @brief	fills a rectangle on the given context using given options
 * @param	ctx - (context_2d *) context to fill a rectangle on
 * @param	rect - (const rect_2d *) rect to be filled
 * @param	color - (const rgba *) color to fill with
 * @param	composite_op - deprecated
 * @retval	NONE
 */
void context_2d_fillRect(context_2d *ctx, const rect_2d *rect, const rgba *color) {
    if (use_single_shader) {
        return;
    }

    draw_textures_flush();
    context_2d_bind(ctx);
    tealeaf_shaders_bind(FILL_RECT_SHADER);
    apply_composite_operation(ctx->globalCompositeOperation[ctx->mvp]);
    rect_2d_vertices in, out;
    rect_2d_to_rect_2d_vertices(rect, &in);
    matrix_3x3_multiply_m_r_r(GET_MODEL_VIEW_MATRIX(ctx), &in, &out);
    float alpha = color->a * ctx->globalAlpha[ctx->mvp];
    // TODO: will pre-multiplied alpha cause a loss-of-precision in color for filling rectangles?
    GLTRACE(glUniform4f(global_shaders[FILL_RECT_SHADER].draw_color, alpha * color->r, alpha * color->g, alpha * color->b, alpha));
    GLTRACE(glVertexAttribPointer(global_shaders[FILL_RECT_SHADER].vertex_coords, 2, GL_FLOAT, GL_FALSE, 0, &out));
    GLTRACE(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    tealeaf_shaders_bind(PRIMARY_SHADER);
}

/**
 * @name	context_2d_fillText
 * @brief	fills text on the given context using given options
 * @param	ctx - (context_2d *) ctx to fill text on
 * @param	img - (texture_2d *) bitmap font texture to use
 * @param	srcRect - (const rect_2d *) source rectangle on the texture to draw from
 * @param	destRect - (const rect_2d *) destination rect to draw to
 * @param	alpha - (float) alpha to draw with
 * @param	composite_op - (int) composite operation to draw with
 * @retval	NONE
 */
void context_2d_fillText(context_2d *ctx, texture_2d *img, const rect_2d *srcRect, const rect_2d *destRect, float alpha) {
    context_2d_bind(ctx);

    if (img && img->loaded) {
        draw_textures_item(ctx, GET_MODEL_VIEW_MATRIX(ctx), img->name, img->width, img->height, img->originalWidth, img->originalHeight, *srcRect, *destRect, *GET_CLIPPING_BOUNDS(ctx), ctx->globalAlpha[ctx->mvp] * alpha, ctx->globalCompositeOperation[ctx->mvp], &ctx->filter_color, ctx->filter_type);
    }
}

/**
 * @name	context_2d_flush
 * @brief	flushes the texture drawing queue
 * @param	ctx - deprecated
 * @retval	NONE
 */
void context_2d_flush(context_2d *ctx) {
    draw_textures_flush();
}

/**
 * @name	context_2d_drawImage
 * @brief	darws an image to the given context using given options
 * @param	ctx - (context_2d *) context to draw to
 * @param	srcTex - (int) deprecated
 * @param	url - (const char *) name of the texture to draw from
 * @param	srcRect - (const rect_2d *) source rectangle on the texture to draw from
 * @param	destRect - (const rect_2d *) destination rect to draw to
 * @param	composite_op - (int) composite operation to draw with
 * @retval	NONE
 */
void context_2d_drawImage(context_2d *ctx, int srcTex, const char *url, const rect_2d *srcRect, const rect_2d *destRect) {
    context_2d_bind(ctx);
    texture_2d *tex = texture_manager_load_texture(texture_manager_get(), url);

    if (tex && tex->loaded) {
        draw_textures_item(ctx, GET_MODEL_VIEW_MATRIX(ctx), tex->name, tex->width, tex->height, tex->originalWidth, tex->originalHeight, *srcRect, *destRect, * GET_CLIPPING_BOUNDS(ctx), ctx->globalAlpha[ctx->mvp], ctx->globalCompositeOperation[ctx->mvp], &ctx->filter_color, ctx->filter_type);
    }
}

void context_2d_setTransform(context_2d *ctx, double m11, double m12, double m21, double m22, double dx, double dy) {
    context_2d_bind(ctx);
    matrix_3x3 *m = GET_MODEL_VIEW_MATRIX(ctx);
    m->m00 = m11;
    m->m01 = m21;
    m->m10 = m12;
    m->m11 = m22;
    m->m02 = dx;
    m->m12 = dy;

}
