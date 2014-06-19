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
 * @file	 events.c
 * @brief
 */
#include "core/events.h"
#include "js/js_events.h"
#include "timestep/timestep_events.h"
#include "core/types.h"
#include "core/core_js.h"

/**
 * @name	core_dispatch_event
 * @brief	dispatches the given event to javascript
 * @param	event - (const char *) holds the json event string to be sent to javascript
 * @retval	NONE
 */
void core_dispatch_event(const char *event) {
    //NO useful events are generated before js is ready
    //therefore only push events when js is ready
    if (js_ready) {
        js_dispatch_event(event);
    }
}

/**
 * @name	core_dispatch_input_event
 * @brief	dispatches an input event
 * @param	id - (int) id of the input
 * @param	type - (int) type of input (down, up, drag, etc)
 * @param	x - (int) x position of the input
 * @param	y - (int) y position of the input
 * @retval	NONE
 */
void core_dispatch_input_event(int id, int type, int x, int y) {
    //NO useful events are generated before js is ready
    //therefore only push events when js is ready
    if (js_ready) {
        timestep_events_push(id, type, x, y);
    }
}
