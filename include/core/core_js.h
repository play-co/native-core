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

#ifndef CORE_JS_H
#define CORE_JS_H

#include "core/types.h"
#include "util/detect.h"

/****************************************************
 * This file exports all functions that tealeaf
 * core needs to know about.
 ****************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// JS Ready flag: Indicates that the JavaScript engine is running
extern bool js_ready;

bool js_init_engine(const char *uri, const char *version);
bool init_js(const char *uri, const char *version);
bool destroy_js();
void eval_str(const char *str);
void js_tick(long dt);
void js_queue_event(const char *evt);
void js_dispatch_event(const char *evt);
void js_on_pause();
void js_on_resume();
void js_set_bundle_id(const char* bundle_id);

typedef struct js_bundle js_bundle_t;
typedef struct application_bundle application_bundle_t;

// Bundles
bool js_ready_for_tick(application_bundle_t *app);
void js_init_bundle(application_bundle_t *app);
void js_enter_bundle(application_bundle_t *app);
void js_exit_bundle(application_bundle_t *app);

#ifdef __cplusplus
}
#endif

#endif
