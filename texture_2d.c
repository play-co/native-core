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
 * @file	 texture_2d.c
 * @brief
 */
#include "core/texture_2d.h"
#include "platform/gl.h"
#include <stdlib.h>
#include <stdio.h>
#include "core/tealeaf_canvas.h"
#include "core/tealeaf_context.h"
#include "core/log.h"
#include "core/image_loader.h"
#include "core/core.h"

// Enable this to print out the texture loader scaling and resizing operations
//#define VERBOSE_LOAD_TEX

static int offscreen_canvas_count = 0;

/**
 * @name	texture_2d_new_from_image
 * @brief	creates a new texture from an already created image
 * @param	url - (char *) name of the already created texture
 * @param	name - (int) gl id of the already created texture
 * @param	width - (int) width of the texture
 * @param	height - (int) height of the texture
 * @param	original_width - (int) original width of the texture
 * @param	original_height - (int) height of the texture
 * @retval	texture_2d* - pointer to the newly created texture
 */
texture_2d *texture_2d_new_from_image(char *url, int name, int width, int height, int original_width, int original_height) {
    texture_2d *tex = (texture_2d *) malloc(sizeof(texture_2d));
    tex->url = url;
    tex->originalWidth = original_width;
    tex->originalHeight = original_height;
    tex->width = width;
    tex->height = height;
    tex->scale = 1;
    tex->name = name;
    tex->original_name = name;
    tex->is_text = false;
    tex->is_canvas = false;
    tex->ctx = NULL;
    tex->saved_data = NULL;
    tex->pixel_data = NULL;
    tex->loaded = false;
    tex->prev = tex->next = NULL;
    tex->num_channels = 4;
    tex->failed = false;
    tex->assumed_texture_bytes = width * height * 4;
    tex->used_texture_bytes = 0;
    tex->compression_type = 0;
    tex->frame_epoch = 0;
    return tex;
}

/**
 * @name	texture_2d_new_from_dimensions
 * @brief	create a new texture with given dimensions
 * @param	width - (int) width to create the texture width
 * @param	height - (int) height to create the texture width
 * @retval	texture_2d* - pointer to the new texture
 */
texture_2d *texture_2d_new_from_dimensions(int width, int height) {
    return texture_2d_new_from_data(width, height, (void *)NULL);
}

/**
 * @name	texture_2d_new_from_url
 * @brief	create a new texture from the given url / filename
 * @param	url - (char *) url / filename to create the texture from
 * @retval	texture_2d* - pointer to the new texture
 */
texture_2d *texture_2d_new_from_url(char *url) {
    texture_2d *tex = (texture_2d *) malloc(sizeof(texture_2d));
    tex->url = url;
    tex->width = 0;
    tex->height = 0;
    tex->scale = 1;
    tex->originalWidth = 0;
    tex->originalHeight = 0;
    tex->name = 0;
    tex->original_name = 0;
    tex->is_text = false;
    tex->is_canvas = false;
    tex->ctx = NULL;
    tex->saved_data = NULL;
    tex->pixel_data = NULL;
    tex->loaded = false;
    tex->prev = tex->next = NULL;
    tex->num_channels = 4;
    tex->failed = false;
    tex->assumed_texture_bytes = 0;
    tex->used_texture_bytes = 0;
    tex->compression_type = 0;
    tex->frame_epoch = 0;
    return tex;
}

/**
 * @name	get_tex_from_data
 * @brief	gets a gl id for a texture with given data
 * @param	w - (int) width of the given image
 * @param	h - (int) height of the given image
 * @param	data - (void *) bytes of the image to create a texture from
 * @retval	int - the gl id representing the created texture
 */
static inline int get_tex_from_data(int w, int h, const void *data) {
    GLuint name;
    GLTRACE(glGenTextures(1, &name));
    GLTRACE(glBindTexture(GL_TEXTURE_2D, name));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLTRACE(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
    return name;
}

/**
 * @name	texture_2d_new_from_data
 * @brief	creates a texture with given data
 * @param	width - (int) width of the texture
 * @param	height - (int) height of the texture
 * @param	data - (void *) bytes of the image to create a texture from
 * @retval	texture_2d* - pointer to the created texture
 */

#define MIN_TEX_SIZE 1 /* Set minimum texture size */

texture_2d *texture_2d_new_from_data(int width, int height, const void *data) {
    GLuint name;
    int w = width, h = height;

    // NOTE: This is a stop-gap measure to prevent crashes on some mobile devices for 0x0 textures
    if (w < MIN_TEX_SIZE) {
        w = MIN_TEX_SIZE;
    }
    if (h < MIN_TEX_SIZE) {
        h = MIN_TEX_SIZE;
    }

    // Power up!
    if ((w & (w-1))) {
        // Bump it up to the next power of 2 (stays the same if already po2)
        // NOTE: Result of w == 0 is 0, but above if-statement avoids this
        --w;
        w |= w >> 1;
        w |= w >> 2;
        w |= w >> 4;
        w |= w >> 8;
        w |= w >> 16;
        ++w;
    }
    if ((h & (h-1))) {
        --h;
        h |= h >> 1;
        h |= h >> 2;
        h |= h >> 4;
        h |= h >> 8;
        h |= h >> 16;
        ++h;
    }

    name = get_tex_from_data(w, h, data);
    texture_2d *tex = (texture_2d *) malloc(sizeof(texture_2d));
    tex->name = name;
    tex->original_name = name;
    tex->originalWidth = width;
    tex->originalHeight = height;
    tex->width = w;
    tex->height = h;
    tex->scale = 1;
    tex->url = (char *)malloc(sizeof(char) * 64);
    snprintf(tex->url, 64, "__canvas__%X", ++offscreen_canvas_count);
    tex->is_text = false;
    tex->is_canvas = true;
    tex->saved_data = NULL;
    tex->pixel_data = NULL;
    tex->loaded = true;
    tex->prev = tex->next = NULL;
    tex->num_channels = 4;
    tex->failed = core_check_gl_error();
    tex->assumed_texture_bytes = width * height * 4;
    tex->used_texture_bytes = 0;
    tex->compression_type = 0;
    tex->frame_epoch = 0;
    return tex;
}

bool texture_2d_can_resize(texture_2d *tex, int width, int height) {
    return width <= tex->width && height <= tex->height;
}

/**
 * @name    texture_2d_resize_unsafe
 * @brief   sets the original width and height. not to be called if new dimensions are greater than texture's width and height, otherwise undefined behavior
 * @param   tex - (texture_2d *) texture to save data from
 * @param   width - new width
 * @param   height - new height
 * @retval  NONE
 */
void texture_2d_resize_unsafe(texture_2d *tex, int width, int height) {
    tex->originalWidth = width;
    tex->originalHeight = height;
}

/**
 * @name	texture_2d_save
 * @brief	saves a texture's byte data from gl to a buffer held by the texture
 * @param	tex - (texture_2d *) texture to save data from
 * @retval	NONE
 */
void texture_2d_save(texture_2d *tex) {
    free(tex->saved_data);
    tex->saved_data = (char *)malloc(sizeof(char) * tex->width * tex->height * 4);
    tealeaf_canvas_context_2d_bind(tex->ctx);
    GLTRACE(glReadPixels(0, 0, tex->width, tex->height, GL_RGBA, GL_UNSIGNED_BYTE, tex->saved_data));
    context_2d *ctx = context_2d_get_onscreen();
    tealeaf_canvas_bind_render_buffer(ctx);
}

/**
 * @name	texture_2d_reload
 * @brief	reloads a texture from it's saved texture byte data
 * @param	tex - (texture_2d *) texture to reload
 * @retval	NONE
 */
void texture_2d_reload(texture_2d *tex) {
    tex->name = get_tex_from_data(tex->width, tex->height, tex->saved_data);
    free(tex->saved_data);
    tex->saved_data = NULL;
}

/**
 * @name	texture_2d_destroy
 * @brief	frees texture and deletes the texture from gl
 * @param	tex - (texture_2d *) texture to destroy
 * @retval	NONE
 */
void texture_2d_destroy(texture_2d *tex) {
    GLTRACE(glDeleteTextures(1, (const GLuint *)&tex->name));
    free(tex->url);
    free(tex->pixel_data);
    free(tex->saved_data);
    free(tex);
}



/*
 * Image post-processor: texture_2d_load_texture_raw()
 *
 * The raw image data needs to be rasterized out to a power-of-two size so that
 * it can be used as a texture in the game.  Furthermore, if half-sizing has
 * been requested then the resulting texture will be half of the original size.
 *
 * IF use_halfsized_textures flagged AND
 *    original width AND height > 64 pixels
 *    THEN perform half-sizing.
 *
 * The URL is only provided for use in debug output prints.
 * The input image data and size is raw compressed PNG/JPEG file data.
 *
 * out: channels, width, height, originalWidth, originalHeight, scale(1/2)
 *
 * Returns rasterized pixel data ready to be used as a texture, or NULL on error.
 */


// Average two color values
#define COLOR_AVG2(x, y) (((unsigned short)( x ) + (unsigned short)( y ) + 1) >> 1)

// Average four color values
#define COLOR_AVG4(x, y, z, w) (((unsigned short)( x ) + (unsigned short)( y ) + (unsigned short)( z ) + (unsigned short)( w ) + 2) >> 2)

// Premultiply alpha value
#define MULT_ALPHA(c, a) (unsigned char)(((unsigned short)( c ) * (unsigned short)( a ) + 128) >> 8)

// Load texture from raw image data, returning null on failure to load
unsigned char *texture_2d_load_texture_raw(const char *url, const void *data, unsigned long sz, int *out_channels, int *out_width, int *out_height, int *out_originalWidth, int *out_originalHeight, int *out_scale, long *out_size, int *out_compression_type) {

    // Initially null pixel data
    unsigned char *pixel_data = NULL;

    //if we don't get data back from this, we need to load from java
    if (!data) {
        // Intentionally not logging an error here
        return NULL;
    }

    // Process file data (PNG/JPEG) into rasterized image data in file format
    int w_old = 0, h_old = 0, ch = 0;
    unsigned char *bits = load_image_from_memory((unsigned char*)data, (long)sz, &w_old, &h_old, &ch, out_size, out_compression_type);
    if (bits == NULL) {
        return NULL;
    }
    *out_channels = ch;
    *out_originalWidth = w_old;
    *out_originalHeight = h_old;

    if (*out_compression_type) {
        *out_width = w_old;
        *out_height = h_old;
        *out_scale = 1;
        return bits;
    } else {
        switch (ch) {
        case 1:
        case 3:
        case 4:
            // We accept 1, 3, and 4 -channel images
            break;
        default:
            // Monochrome: 2 byte/pixel: first for color, second for alpha
            // TODO: Needs to be converted up to RGBA to work with OpenGL
            LOG("{resources} WARNING: Unable to work with %d-channel image. Please convert this file to another format: %s", ch, url);
            free(bits);
            return NULL;
        }
    }

    // Catch invalid image dimensions
    if (w_old <= 0 || h_old <= 0) {
        LOG("{resources} WARNING: Invalid image dimensions w=%d, h=%d", w_old, h_old);
        free(bits);
        return NULL;
    }

    // Now we post-process the image data into our internal memory format:

    int w = w_old, h = h_old;
    bool reformatted = false;
#ifdef VERBOSE_LOAD_TEX
    bool debug_is_half = false, debug_is_po2_w = false, debug_is_po2_h = false;
#endif

    // If texture should be half-sized,
    int scale = 1;
    if (use_halfsized_textures && (h > 64 && w > 64)) {
        reformatted = true;
        scale = 2;

        // Scale width and height if needed, rounding up (must happen)
        w = (w + 1) >> 1;
        h = (h + 1) >> 1;

#ifdef VERBOSE_LOAD_TEX
        debug_is_half = true;
#endif
    }
    *out_scale = scale;

    // Width: If at least 2 bits are set (is not power-of-2),
    // NOTE: This is unlikely so the if-statement is worthwhile
    if ((w & (w-1))) {
        // Bump it up to the next power of 2 (stays the same if already po2)
        // NOTE: Result of w == 0 is 0
        --w;
        w |= w >> 1;
        w |= w >> 2;
        w |= w >> 4;
        w |= w >> 8;
        w |= w >> 16;
        ++w;
        reformatted = true;

#ifdef VERBOSE_LOAD_TEX
        debug_is_po2_w = true;
#endif
    }

    // Height: If at least 2 bits are set (is not power-of-2),
    if ((h & (h-1))) {
        // Bump it up to the next power of 2 (stays the same if already po2)
        --h;
        h |= h >> 1;
        h |= h >> 2;
        h |= h >> 4;
        h |= h >> 8;
        h |= h >> 16;
        ++h;
        reformatted = true;

#ifdef VERBOSE_LOAD_TEX
        debug_is_po2_h = true;
#endif
    }

#ifdef VERBOSE_LOAD_TEX
    LOG("{resources} Loading texture url=%s, originalSize=%dx%d, channelCount=%d, newSize=%dx%d, half=%d,po2w=%d,po2h=%d", url, w_old, h_old, ch, w, h, (int)debug_is_half, (int)debug_is_po2_w, (int)debug_is_po2_h);
#endif

    // Store resulting new width and height and scale
    *out_width = w << (scale - 1);
    *out_height = h << (scale - 1);

    // If the image was reformatted,
    if (reformatted) {
#ifdef __ANDROID__
        unsigned char *output = memalign(8, w * h * ch);
        if (!output) {
#else
        unsigned char *output;
        if (0 != posix_memalign((void**)&output, 8, w * h * ch)) {
#endif
            LOG("{resources} WARNING: Unable to allocate reformatted image w=%d, h=%d", w, h);
            free(bits);
            return NULL;
        }
        const unsigned char *rowi = bits;
        unsigned char *rowo = output;
        int x, y;

        // If scaling,
        if (scale == 2) {
            // If RGBA,
            if (ch == 4) {
                const int OLD_STRIDE = w_old << 2;
                const int RIGHT_GAP = (w - ((w_old+1)>>1)) << 2;
                const int ROUND_W_OLD = w_old & ~1;
                const int ROUND_H_OLD = h_old & ~1;
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Scaling RGBA oddWidth=%d, oddHeight=%d, oldStride=%d, rightGap=%d, roundWOld=%d, roundHOld=%d", (int)(w_old&1), (int)(h_old&1), OLD_STRIDE, RIGHT_GAP, ROUND_W_OLD, ROUND_H_OLD);
#endif

                for (y = 0; y < ROUND_H_OLD; y += 2, rowi += OLD_STRIDE) {
                    // Average 2x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        // Accumulate pixels with color data, ignore the clear ones
                        unsigned short a0 = rowi[3], a1 = rowi[7], a2 = rowi[OLD_STRIDE+3], a3 = rowi[OLD_STRIDE+7];
                        unsigned short a = 0, r = 0, g = 0, b = 0, acnt = 0;
                        if (a0) {
                            a += a0;
                            ++acnt;
                            r += rowi[0];
                            g += rowi[1];
                            b += rowi[2];
                        }
                        if (a1) {
                            a += a1;
                            ++acnt;
                            r += rowi[4];
                            g += rowi[5];
                            b += rowi[6];
                        }
                        if (a2) {
                            a += a2;
                            ++acnt;
                            r += rowi[OLD_STRIDE];
                            g += rowi[OLD_STRIDE+1];
                            b += rowi[OLD_STRIDE+2];
                        }
                        if (a3) {
                            a += a3;
                            ++acnt;
                            r += rowi[OLD_STRIDE+4];
                            g += rowi[OLD_STRIDE+5];
                            b += rowi[OLD_STRIDE+6];
                        }

                        // Average the resulting colors
                        switch (acnt) {
                        case 2:
                            a = (a + 1) >> 1;
                            r = (r + 1) >> 1;
                            g = (g + 1) >> 1;
                            b = (b + 1) >> 1;
                            break;
                        case 3:
                            a = (a + 1) / 3;
                            r = (r + 1) / 3;
                            g = (g + 1) / 3;
                            b = (b + 1) / 3;
                            break;
                        case 4:
                            a = (a + 2) >> 2;
                            r = (r + 2) >> 2;
                            g = (g + 2) >> 2;
                            b = (b + 2) >> 2;
                            break;
                        default:
                        case 0:
                        case 1:
                            break;
                        }

                        // Premultiply alpha
                        rowo[0] = MULT_ALPHA(r, a);
                        rowo[1] = MULT_ALPHA(g, a);
                        rowo[2] = MULT_ALPHA(b, a);
                        rowo[3] = (unsigned char)a;

                        rowi += 8;
                        rowo += 4;
                    }

                    // Average final odd column with row below it
                    if (w_old & 1) {
                        // Accumulate pixels with color data, ignore the clear ones
                        unsigned short a0 = rowi[3], a2 = rowi[OLD_STRIDE+3];
                        unsigned short a = 0, r = 0, g = 0, b = 0, acnt = 0;
                        if (a0) {
                            a += a0;
                            ++acnt;
                            r += rowi[0];
                            g += rowi[1];
                            b += rowi[2];
                        }
                        if (a2) {
                            a += a2;
                            ++acnt;
                            r += rowi[OLD_STRIDE];
                            g += rowi[OLD_STRIDE+1];
                            b += rowi[OLD_STRIDE+2];
                        }

                        // Average the resulting colors
                        if (acnt == 2) {
                            a = (a + 1) >> 1;
                            r = (r + 1) >> 1;
                            g = (g + 1) >> 1;
                            b = (b + 1) >> 1;
                        }

                        // Premultiply alpha
                        rowo[0] = MULT_ALPHA(r, a);
                        rowo[1] = MULT_ALPHA(g, a);
                        rowo[2] = MULT_ALPHA(b, a);
                        rowo[3] = (unsigned char)a;

                        rowi += 4;
                        rowo += 4;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Average final odd row with itself
                if (h_old & 1) {
                    // Average 1x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        // Accumulate pixels with color data, ignore the clear ones
                        unsigned short a0 = rowi[3], a1 = rowi[7];
                        unsigned short a = 0, r = 0, g = 0, b = 0, acnt = 0;
                        if (a0) {
                            a += a0;
                            ++acnt;
                            r += rowi[0];
                            g += rowi[1];
                            b += rowi[2];
                        }
                        if (a1) {
                            a += a1;
                            ++acnt;
                            r += rowi[4];
                            g += rowi[5];
                            b += rowi[6];
                        }

                        // Average the resulting colors
                        if (acnt == 2) {
                            a = (a + 1) >> 1;
                            r = (r + 1) >> 1;
                            g = (g + 1) >> 1;
                            b = (b + 1) >> 1;
                        }

                        // Premultiply alpha
                        rowo[0] = MULT_ALPHA(r, a);
                        rowo[1] = MULT_ALPHA(g, a);
                        rowo[2] = MULT_ALPHA(b, a);
                        rowo[3] = (unsigned char)a;

                        rowi += 8;
                        rowo += 4;
                    }

                    // Direct copy final pixel
                    if (w_old & 1) {
                        // Premultiply alpha
                        unsigned short a = rowi[3];
                        rowo[0] = MULT_ALPHA(rowi[0], a);
                        rowo[1] = MULT_ALPHA(rowi[1], a);
                        rowo[2] = MULT_ALPHA(rowi[2], a);
                        rowo[3] = (unsigned char)a;

                        //rowi += 4;
                        rowo += 4;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                const int BOTTOM_GAP = (h - ((h_old+1)>>1)) * (w << 2);
                memset(rowo, 0, BOTTOM_GAP);
            } else if (ch == 3) { // RGB
                const int OLD_STRIDE = w_old * 3;
                const int RIGHT_GAP = (w - ((w_old+1)>>1)) * 3;
                const int ROUND_W_OLD = w_old & ~1;
                const int ROUND_H_OLD = h_old & ~1;
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Scaling RGB oddWidth=%d, oddHeight=%d, oldStride=%d, rightGap=%d, roundWOld=%d, roundHOld=%d", (int)(w_old&1), (int)(h_old&1), OLD_STRIDE, RIGHT_GAP, ROUND_W_OLD, ROUND_H_OLD);
#endif

                for (y = 0; y < ROUND_H_OLD; y += 2, rowi += OLD_STRIDE) {
                    // Average 2x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        rowo[0] = COLOR_AVG4(rowi[0], rowi[3], rowi[OLD_STRIDE], rowi[OLD_STRIDE+3]);
                        rowo[1] = COLOR_AVG4(rowi[1], rowi[4], rowi[OLD_STRIDE+1], rowi[OLD_STRIDE+4]);
                        rowo[2] = COLOR_AVG4(rowi[2], rowi[5], rowi[OLD_STRIDE+2], rowi[OLD_STRIDE+5]);

                        rowi += 6;
                        rowo += 3;
                    }

                    // Average final odd column with row below it
                    if (w_old & 1) {
                        rowo[0] = COLOR_AVG2(rowi[0], rowi[OLD_STRIDE]);
                        rowo[1] = COLOR_AVG2(rowi[1], rowi[OLD_STRIDE+1]);
                        rowo[2] = COLOR_AVG2(rowi[2], rowi[OLD_STRIDE+2]);

                        rowi += 3;
                        rowo += 3;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Average final odd row with itself
                if (h_old & 1) {
                    // Average 1x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        rowo[0] = COLOR_AVG2(rowi[0], rowi[3]);
                        rowo[1] = COLOR_AVG2(rowi[1], rowi[4]);
                        rowo[2] = COLOR_AVG2(rowi[2], rowi[5]);

                        rowi += 6;
                        rowo += 3;
                    }

                    // Direct copy final pixel
                    if (w_old & 1) {
                        rowo[0] = rowi[0];
                        rowo[1] = rowi[1];
                        rowo[2] = rowi[2];

                        //rowi += 3;
                        rowo += 3;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                const int BOTTOM_GAP = (h - ((h_old+1)>>1)) * w * 3;
                memset(rowo, 0, BOTTOM_GAP);
            } else { // ch == 1
                // Monochrome: 1 byte/pixel color
                // Natively compatible with GL_LUMINANCE
                const int OLD_STRIDE = w_old;
                const int RIGHT_GAP = (w - ((w_old+1)>>1));
                const int ROUND_W_OLD = w_old & ~1;
                const int ROUND_H_OLD = h_old & ~1;
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Scaling Monochrome oddWidth=%d, oddHeight=%d, oldStride=%d, rightGap=%d, roundWOld=%d, roundHOld=%d", (int)(w_old&1), (int)(h_old&1), OLD_STRIDE, RIGHT_GAP, ROUND_W_OLD, ROUND_H_OLD);
#endif

                for (y = 0; y < ROUND_H_OLD; y += 2, rowi += OLD_STRIDE) {
                    // Average 2x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        rowo[0] = COLOR_AVG4(rowi[0], rowi[1], rowi[OLD_STRIDE], rowi[OLD_STRIDE+1]);

                        rowi += 2;
                        rowo += 1;
                    }

                    // Average final odd column with row below it
                    if (w_old & 1) {
                        rowo[0] = COLOR_AVG2(rowi[0], rowi[OLD_STRIDE]);

                        rowi += 1;
                        rowo += 1;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Average final odd row with itself
                if (h_old & 1) {
                    // Average 1x2 blocks
                    for (x = 0; x < ROUND_W_OLD; x += 2) {
                        rowo[0] = COLOR_AVG2(rowi[0], rowi[1]);

                        rowi += 2;
                        rowo += 1;
                    }

                    // Direct copy final pixel
                    if (w_old & 1) {
                        rowo[0] = rowi[0];

                        //rowi += 1;
                        rowo += 1;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                const int BOTTOM_GAP = (h - ((h_old+1)>>1)) * w;
                memset(rowo, 0, BOTTOM_GAP);
            }
        } else { // Unscaled: Made a power of 2
            // If RGBA,
            if (ch == 4) {
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Unscaled RGBA");
#endif
                const int RIGHT_GAP = (w - w_old) << 2;

                for (y = 0; y < h_old; ++y) {
                    for (x = 0; x < w_old; ++x) {
                        // Copy and pre-multiply alpha
                        unsigned short a = rowi[3];
                        rowo[0] = MULT_ALPHA(rowi[0], a);
                        rowo[1] = MULT_ALPHA(rowi[1], a);
                        rowo[2] = MULT_ALPHA(rowi[2], a);
                        rowo[3] = a;

                        rowi += 4;
                        rowo += 4;
                    }

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                memset(rowo, 0, (h - h_old) * (w << 2));
            } else if (ch == 3) { // RGB
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Unscaled RGB");
#endif
                const int W_OLD_BYTES = w_old * 3;
                const int W_BYTES = w * 3;
                const int RIGHT_GAP = W_BYTES - W_OLD_BYTES;

                for (y = 0; y < h_old; ++y, rowi += W_OLD_BYTES) {
                    // Copy full row without changes
                    memcpy(rowo, rowi, W_OLD_BYTES);
                    rowo += W_OLD_BYTES;

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                memset(rowo, 0, (h - h_old) * W_BYTES);
            } else { // ch == 1
#ifdef VERBOSE_LOAD_TEX
                LOG("{resources} Processing: Unscaled Monochrome");
#endif
                const int W_OLD_BYTES = w_old;
                const int W_BYTES = w;
                const int RIGHT_GAP = W_BYTES - W_OLD_BYTES;

                for (y = 0; y < h_old; ++y, rowi += W_OLD_BYTES) {
                    // Copy full row without changes
                    memcpy(rowo, rowi, W_OLD_BYTES);
                    rowo += W_OLD_BYTES;

                    // Zero out the right gap
                    memset(rowo, 0, RIGHT_GAP);
                    rowo += RIGHT_GAP;
                }

                // Zero out the bottom gap
                memset(rowo, 0, (h - h_old) * W_BYTES);
            }
        }

        free(bits);

        pixel_data = output;
    } else {
        // Re-use image data already at the right size and scale:

        if (ch == 4) {
#ifdef VERBOSE_LOAD_TEX
            LOG("{resources} Processing: Unformatted RGBA");
#endif
            unsigned char *row = bits;
            unsigned int bytes = w * h;

            while (bytes--) {
                // Premultiply alpha
                unsigned short a = row[3];
                row[0] = MULT_ALPHA(row[0], a);
                row[1] = MULT_ALPHA(row[1], a);
                row[2] = MULT_ALPHA(row[2], a);

                row += 4;
            }
        }
        // 1 and 3 -channel images do not need any modification here

        pixel_data = bits;
    }

    return pixel_data;
}

