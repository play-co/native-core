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

#ifndef DEVICE_H
#define DEVICE_H

#include "core/util/detect.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *device_global_id();
const char *device_info();
int device_total_memory();
void device_hide_splash();
float device_get_text_scale();
void device_set_text_scale(float scale);
	
#ifdef __cplusplus
}
#endif

#endif
