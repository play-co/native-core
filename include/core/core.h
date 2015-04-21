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

#ifndef CORE_H
#define CORE_H

#include "core/list.h"
#include "core/texture_2d.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int use_halfsized_textures;

void core_init(const char *entry_point,
               const char *tcp_host,
               const char *code_host,
               int tcp_port,
               int code_port,
               const char *source_dir,
               bool remote_loading,
			   const char *splash,
               const char *simulate_id);
bool core_init_js(const char *uri, const char *version);
void core_report_gl_error(int error_code);
bool core_check_gl_error();
void core_init_gl(int framebuffer_name, int tex_name);
void core_hide_preloader();
void core_on_screen_resize(int width, int height);
void core_run();
void core_destroy();
void core_reset();
void core_tick(long dt);

bool core_js_engine_init(const char* uri, const char* version);
bool core_js_eval(const char* file);
void core_destroy_gl();
void core_reinit();

void core_run_bundle(const char *name);
void core_load_bundle(const char *name, const char *remote_url);
void core_exit_bundle(const char *name);


#ifdef __cplusplus
}
#endif

#endif
