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

#ifndef TIMESTEP_INPUT_EVENT_H
#define TIMESTEP_INPUT_EVENT_H

#include "core/util/detect.h"

typedef struct input_event_t {
	int id;
	int type;
	int x;
	int y;
} input_event;

typedef struct input_event_list_t {
	input_event *events;
	unsigned int count;
} input_event_list;

CEXPORT void timestep_events_push(int id, int type, int x, int y);
CEXPORT input_event_list timestep_events_get(); 
CEXPORT void timestep_events_shutdown();

#endif // TIMESTEP_INPUT_EVENT_H
