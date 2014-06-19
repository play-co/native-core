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
 * @file	 core.c
 * @brief
 */
#include "core/types.h"
#include "core/core.h"
#include "core/config.h"
#include "core/rgba.h"
#include "core/texture_manager.h"
#include "core/tealeaf_canvas.h"
#include "core/tealeaf_context.h"
#include "core/tealeaf_shaders.h"
#include "core/url_loader.h"
#include "core/log.h"
#include "core/events.h"
#include "core/core_js.h"
#include "core/platform/resource_loader.h"
#include "core/platform/sound_manager.h"
#include "core/timer.h"
#include "core/platform/native.h"
#include "platform/http.h"
#include "platform/device.h"
#include <stdio.h>

#define MIN_SIZE_TO_HALFSIZE 320

gl_error *gl_errors_hash = NULL;
static int m_framebuffer_name = -1;

/**
 * @name	run_file
 * @brief	reads and runs javascript found in the given file
 * @param	filename - (const char*) filename of the file to run
 * @retval	bool - (true | false) depending on whether running the file was successful
 */
static inline bool run_file(const char *filename) {
    char *contents = core_load_url(filename);

    if (contents) {
        eval_str(contents);

        LOG("{core} Evaluated JavaScript from %s", filename);

        free(contents);
        return true;
    } else {
        LOG("{core} WARNING: Error reading JavaScript from %s", filename);
        return false;
    }
}

/**
 * @name	core_init
 * @brief	initilizes the config object with given options
 *			See config.c for examples for options
 * @param	entry_point - (const char*) entry point
 * @param	tcp_host - (const char*) tcp host
 * @param	code_host - (const char*) code host
 * @param	tcp_port - (int) representing the tcp port
 * @param	code_port - (int) representing the code port
 * @param	source_dir - (const char*) representing where the source directory is located
 * @param	width - (int) representing the width of the screen
 * @param	height - (int) representing the height of the screen
 * @param	remote_loading - (bool) representing whether remote loading is on / orr
 * @param	splash - (const char*) splash screen path
 * @param	simulate_id - (const char*) representing the id of the game to be simulated
 * @retval	NONE
 */
void core_init(const char *entry_point,
               const char *tcp_host,
               const char *code_host,
               int tcp_port,
               int code_port,
               const char *source_dir,
               int width,
               int height,
               bool remote_loading,
               const char *splash,
               const char *simulate_id) {
    config_set_remote_loading(remote_loading);
    config_set_entry_point(entry_point);
    config_set_tcp_host(tcp_host);
    config_set_code_host(code_host);
    config_set_tcp_port(tcp_port);
    config_set_code_port(code_port);
    config_set_screen_width(width);
    config_set_screen_height(height);
    config_set_splash(splash);
    config_set_simulate_id(simulate_id);
    // http_init();
    // register default HTML color names
    rgba_init();
    // make checks for halfsized images
    resource_loader_initialize(source_dir);

    if (width <= MIN_SIZE_TO_HALFSIZE || height <= MIN_SIZE_TO_HALFSIZE) {
        set_halfsized_textures(true);
    } else {
        set_halfsized_textures(false);
    }

    texture_manager_load_texture(texture_manager_get(), config_get_splash());

    LOG("{core} Initialization complete");
}

/**
 * @name	core_init_gl
 * @brief	initializes shaders, canvas, and framebuffer
 * @param	framebuffer_name - (int) gl id of the onscreen framebuffer
 * @retval	NONE
 */
void core_init_gl(int framebuffer_name) {
    LOG("{core} Initializing OpenGL");

    tealeaf_shaders_init();
    m_framebuffer_name = framebuffer_name;

    // If frame buffer id was invalid,
    if (m_framebuffer_name < 0) {
        LOG("{core} WARNING: Framebuffer used to init GL was invalid");
    }

    // Canvas must be initialized after shaders
    tealeaf_canvas_init(m_framebuffer_name);
}

/**
 * @name	core_init_js
 * @brief	initilizes js options and runs the packed javascript game code
 * @param	uri - (const char*)  ____
 * @param	version - (const char*) ____
 * @retval	bool - (true | false) depending on whether running the javascript source was successful
 */
bool core_init_js(const char *uri, const char *version) {
    core_timer_clear_all();

    init_js(uri, version);
    return run_file("native.js");
}

/**
 * @name	core_run
 * @brief	_____
 * @retval	NONE
 */
void core_run() {
    char buf[64];
    snprintf(buf, sizeof(buf), "jsio('import %s;')", config_get_entry_point());
    eval_str(buf);
}

/*GLOBAL*/
bool show_preload = true;
static int preload_hide_frame_count = 0;

static bool do_sizing = true, rotate = false;
static rect_2d tex_size;
static rect_2d size;

/**
 * @name	calculate_size
 * @brief	calclates source and destination size of the loading image
 * @param	tex - (texture_2d*) load screen texture
 * @param	rotate - rotate 90 degrees
 * @retval	NONE
 */
void calculate_size(texture_2d *tex, bool rotate) {
    tealeaf_canvas *canvas = tealeaf_canvas_get();
    int scale = tex->scale;
    int width = canvas->framebuffer_width;
    int height = canvas->framebuffer_height;
    if (rotate) {
        int t = width;
        width = height;
        height = t;
    }
    float vertical = height * 1.0f / tex->originalHeight;
    float horizontal = width * 1.0f / tex->originalWidth;
    float ratio = (vertical > horizontal ? vertical : horizontal);
    float offsetX = (width - ratio * tex->originalWidth) / 2;
    float offsetY = (height - ratio * tex->originalHeight) / 2;
    tex_size.x = 0;
    tex_size.y = 0;
    tex_size.width = tex->originalWidth * scale;
    tex_size.height = tex->originalHeight * scale;
    size.x = offsetX;
    size.y = offsetY;
    size.width = ratio * tex->originalWidth * scale;
    size.height = ratio * tex->originalHeight * scale;
}

/**
 * @name	core_tick
 * @brief	moves the game forward by a single tick, defined by a time delta of
 *			last tick to this tick
 * @param	dt - (long) elapsed time from last tick to this tick in milliseconds
 * @retval	NONE
 */
void core_tick(long dt) {
    if (js_ready) {
        core_timer_tick(dt);
        js_tick(dt);
    }

    // Tick the texture manager (load pending textures)
    texture_manager_tick(texture_manager_get());
    /*
     * we need to wait 2 frames before removing the preloader after we get the
     * core_hide_preloader call from JS.  Only on the second frame after the
     * callback are the images that were preloaded actually drawn.
     */

    if (show_preload || preload_hide_frame_count < 2) {
        //if we've gotten the core_hide_preloader cb, start counting frames
        if (!show_preload) {
            preload_hide_frame_count++;

            // May have never loaded the splash image, so hide splash here too
            device_hide_splash();
        }

        // If splash is defined,
        const char *splash = config_get_splash();
        if (splash) {
            texture_2d *tex = texture_manager_get_texture(texture_manager_get(), splash);
            if (!tex) {
                tex = texture_manager_load_texture(texture_manager_get(), splash);
            }

            if (tex && tex->loaded) {
                if (do_sizing) {
                    // Calculate rotation
                    tealeaf_canvas *canvas = tealeaf_canvas_get();
                    int canvas_width = canvas->framebuffer_width;
                    int canvas_height = canvas->framebuffer_height;
                    rotate = canvas_width > canvas_height;
                    rotate ^= tex->originalWidth > tex->originalHeight;

                    calculate_size(tex, rotate);
                    do_sizing = false;
                }

                context_2d *ctx = context_2d_get_onscreen(tealeaf_canvas_get());
                context_2d_loadIdentity(ctx);
                context_2d_clear(ctx);
                if (rotate) {
                    context_2d_save(ctx);
                    context_2d_translate(ctx, size.y + (size.height)/2.f/tex->scale, size.x + (size.width)/2.f/tex->scale);
                    context_2d_rotate(ctx, (tex->originalWidth > tex->originalHeight)? -3.14f/2.f : 3.14f/2.f);
                    context_2d_translate(ctx, -size.x -(size.width)/2.f/tex->scale, -size.y - (size.height)/2.f/tex->scale);
                }
                context_2d_setGlobalCompositeOperation(ctx, source_over);
                context_2d_drawImage(ctx, 0, splash, &tex_size, &size);
                if (rotate) {
                    context_2d_restore(ctx);
                }
                // we're the first, last, and only thing to draw, so flush the buffer
                context_2d_flush(ctx);

                device_hide_splash();
            }
        }
    }

    // check the gl error and send it to java to be logged
    if (js_ready) {
        core_check_gl_error();
    }
}

/**
 * @name	core_hide_preloader
 * @brief	hides the preloader from the screen
 * @retval	NONE
 */
void core_hide_preloader() {
    show_preload = false;
}

/**
 * @name	core_on_screen_resize
 * @brief	resizes the onscreen canvas to the given width and height
 * @param	width -	(int) width to resize to
 * @param	height - (int) height to resize to
 * @retval	NONE
 */
void core_on_screen_resize(int width, int height) {
    tealeaf_canvas_resize(width, height);
}

/**
 * @name	core_destroy
 * @brief	destroys the running js and the texture manager
 * @retval	NONE
 */
void core_destroy() {
    destroy_js();
    texture_manager_destroy(texture_manager_get());
    sound_manager_halt();
}

/**
 * @name	core_reset
 * @brief	destroys core as well as resets preset variables
 * @retval	NONE
 */
void core_reset() {
    core_destroy();
    do_sizing = true;
    show_preload = true;
    preload_hide_frame_count = 0;
}

/**
 * @name	core_check_gl_error
 * @brief	check's for a gl error and reports it to the platform
 * @retval	(bool) whether or not there was an error
 */
bool core_check_gl_error() {
    // check the gl error and send it to java to be logged
    int error_code = glGetError();
    if (error_code != 0) {
        LOG("{core} WARNING: OpenGL error %d", error_code);

        // warn the texture manager if necessary
        if (error_code == GL_OUT_OF_MEMORY) {
            texture_manager_memory_warning();
        }

        // check if error is in the hash, report each error only once
        gl_error *error = NULL;
        HASH_FIND_INT(gl_errors_hash, &error_code, error);
        if (!error) {
            report_gl_error(error_code, &gl_errors_hash, false);
        }

        return true;
    } else {
        return false;
    }
}
