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

#ifndef NATIVE_H
#define NATIVE_H

#include "core/util/detect.h"
#include "core/deps/uthash/uthash.h"

typedef struct gl_error_t {
	int error_code;
	UT_hash_handle hh;
} gl_error;


void start_game(const char *appid);
void apply_update();
const char *get_market_url();
const char *get_app_version();
bool native_send_activity_to_back();
char *get_storage_directory();
void upload_contacts();
void upload_device_info();
const char *get_install_referrer();
CEXPORT void report_gl_error(int error_code, gl_error **errors_hash, bool unrecoverable);
CEXPORT void set_halfsized_textures(bool on);
CEXPORT void native_stay_awake(bool on);
void native_reload();
char *native_call(const char *method, const char *args);
const char *get_version_code();

// Call these from a native thread when entering and leaving, to perform the
// startup and cleanup required.
CEXPORT void native_enter_thread();
CEXPORT void native_leave_thread();

#endif
