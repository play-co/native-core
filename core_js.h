/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the Game Closure SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CORE_JS_H
#define CORE_JS_H

#include "core/util/detect.h"
#include "core/types.h"

/****************************************************
 * This file exports all functions that tealeaf
 * core needs to know about.
 ****************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// JS Ready flag: Indicates that the JavaScript engine is running
extern bool js_ready;

bool init_js(const char *uri, const char *version);
bool destroy_js();
void eval_str(const char *str);
void js_tick(int dt);
void js_dispatch_event(const char *evt);
void js_on_pause();
void js_on_resume();

#ifdef __cplusplus
}
#endif

#endif
